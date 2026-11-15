#ifndef GRAN_RHI_METAL_INTERNAL_BACKEND_IMPL_HPP
#define GRAN_RHI_METAL_INTERNAL_BACKEND_IMPL_HPP

/*
** Внутренности бэкенда Metal: только для файлов .mm из src/rhi/metal (Objective-C++ с ARC).
**
** Владение объектами GPU. Правило gpu_rules.md «владение только через свои обёртки» здесь выполняет ARC:
** поле `id<MTL…>` — сильная ссылка, присваивание nil или выход из области освобождает объект, копирование
** увеличивает счётчик. Своя обёртка поверх ARC ничего бы не добавила. Командный буфер Metal сам удерживает
** ресурсы, которые в него записаны (буфер создаётся не через commandBufferWithUnretainedReferences), поэтому
** освобождённая текстура или старый буфер экземпляров живут, пока их читает GPU, — роль DeferredReleaseQueue.
**
** Синхронизация. Кадров в полёте — FRAMES_IN_FLIGHT; семафор ждёт свободный кадр, у каждого кадра свои буферы
** экземпляров и отсечений. Константы кадра идут через setVertexBytes/setFragmentBytes (копируются в командный
** буфер). Загрузка текстур — не replaceRegion (гонка с кадром в полёте), а копия из промежуточного буфера
** в начале следующего командного буфера: порядок с отрисовкой задаёт сама очередь.
**
** Обработчик завершения командного буфера работает в чужом потоке: он только пишет в SharedState под
** мьютексом и отпускает семафор, исключения через него не проходят (noexcept).
**
** Metal backend internals: for the .mm files of src/rhi/metal only (Objective-C++ with ARC).
**
** GPU object ownership. The gpu_rules.md rule "ownership only through own wrappers" is fulfilled by ARC here:
** an `id<MTL…>` field is a strong reference, assigning nil or leaving the scope releases the object, copying
** increments the count. An own wrapper over ARC would add nothing. A Metal command buffer itself retains the
** resources recorded into it (the buffer is not created with commandBufferWithUnretainedReferences), so
** a released texture or an old instance buffer lives while the GPU reads it — the role of DeferredReleaseQueue.
**
** Synchronization. There are FRAMES_IN_FLIGHT frames in flight; a semaphore waits for a free frame, each frame
** has its own instance and clip buffers. The frame constants go through setVertexBytes/setFragmentBytes (copied into
** the command buffer). Texture uploads are not replaceRegion (a race with a frame in flight) but a copy from
** a staging buffer at the start of the next command buffer: the queue itself orders it with the drawing.
**
** The command buffer completion handler runs in a foreign thread: it only writes into SharedState under
** a mutex and releases the semaphore, no exceptions pass through it (noexcept).
**
** Окно (M6). Кадр всегда рисуется в свою поверхность (_surface): из неё же читают размытие подложки и readPixels.
** С окном (nativeWindow — CAMetalLayer вида Cocoa) поверхность и все конвейеры — в формате BGRA8 (CAMetalLayer
** не умеет RGBA8), а в конце командного буфера поверхность копируется в drawable слоя и показывается
** (presentDrawable). Drawable удерживает командный буфер; пул автоосвобождения вокруг кадра обязателен: без него
** drawable, выданные nextDrawable, не возвращались бы слою, и через три кадра nextDrawable ждал бы вечно.
**
** The window (M6). The frame is always drawn into the own surface (_surface): the backdrop blur and readPixels read it
** too. With a window (nativeWindow is the CAMetalLayer of a Cocoa view) the surface and all the pipelines are in the
** BGRA8 format (CAMetalLayer cannot do RGBA8), and at the end of the command buffer the surface is copied into the layer
** drawable and shown (presentDrawable). The command buffer retains the drawable; an autorelease pool around the frame
** is mandatory: without it the drawables given by nextDrawable would not return to the layer, and after three frames
** nextDrawable would wait forever.
*/
#if !defined(__OBJC__) || !__has_feature(objc_arc)
#error "metal_backend_impl.hpp: only for Objective-C++ with ARC (-fobjc-arc)"
#endif

#include "gran/rhi/metal/metal_backend.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace gran::rhi::metal
{
    // Кадров в полёте, уровней размытия подложки, логических слотов текстур (как в D3D11)
    // Frames in flight, backdrop blur levels, logical texture slots (as in D3D11)
    constexpr std::uint32_t FRAMES_IN_FLIGHT = 2;
    constexpr std::uint32_t BLUR_LEVELS = 6;
    constexpr std::uint32_t MAX_TEXTURES = 4096;
    // Предел буфера экземпляров или отсечений одного кадра, байт (как в D3D11 и Vulkan)
    // The limit of one frame's instance or clip buffer, bytes (as in D3D11 and Vulkan)
    constexpr std::size_t MAX_FRAME_BUFFER = 128u * 1024u * 1024u;

    // Текстура и её размеры; bytes — байт на пиксель (1 для R8, 4 для RGBA8)
    // A texture and its size; bytes — bytes per pixel (1 for R8, 4 for RGBA8)
    struct Texture
    {
        id<MTLTexture> resource = nil;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t bytes = 4;
    };

    // Буферы одного кадра в полёте: экземпляры и отсечения (общая память CPU и GPU)
    // The buffers of one frame in flight: the instances and the clips (memory shared by CPU and GPU)
    struct FrameBuffers
    {
        id<MTLBuffer> instances = nil;
        id<MTLBuffer> clips = nil;
    };

    // Отложенная загрузка: строки уже упакованы в staging с шагом pitch, копия — в начале следующего кадра
    // A deferred upload: the rows are already packed into staging with the pitch step, the copy is at the next frame start
    struct PendingUpload
    {
        id<MTLBuffer> staging = nil;
        id<MTLTexture> texture = nil;
        TextureRegion region;
        std::uint32_t pitch = 0;
    };

    // Состояние, общее с обработчиками завершения (чужой поток): живёт, пока жив хотя бы один обработчик
    // The state shared with the completion handlers (a foreign thread): lives while at least one handler lives
    struct SharedState
    {
        dispatch_semaphore_t inFlight = nil;
        std::mutex mutex;
        std::vector<std::string> messages;
        std::string lostReason;
        std::atomic<bool> lost{false};
    };

    // Ошибка вызова Metal: ok == false → GpuException (код NSError, текст — из каталога сообщений)
    // A Metal call error: ok == false → GpuException (the NSError code, the text — from the message catalog)
    void check(bool ok, NSError* error, const char* call, const char* file, int line);
    // Неверный аргумент от вызывающего кода (слот, область, кадр) / An invalid argument from the caller (slot, region, frame)
    [[noreturn]] void invalid(const char* resource);
    // Текстура RGBA8 или R8 в памяти GPU; target — формат цели отрисовки (MTLPixelFormatInvalid — не цель, формат из desc)
    // An RGBA8 or R8 texture in GPU memory; target — the render target format (MTLPixelFormatInvalid — not a target, the format from desc)
    void makeTexture(id<MTLDevice> device, Texture& texture, const TextureDesc& desc, MTLPixelFormat target);
    // Строки, прочитанные в формате поверхности, → RGBA8 (BGRA8 окна меняет местами R и B) (metal_present.mm)
    // The rows read in the surface format → RGBA8 (the window BGRA8 swaps R and B) (metal_present.mm)
    void toRgba(std::vector<std::uint8_t>& pixels, MTLPixelFormat format) noexcept;
    // Итог командного буфера → SharedState; вызывается из обработчика завершения
    // A command buffer result → SharedState; called from the completion handler
    void completed(SharedState& state, id<MTLCommandBuffer> buffer) noexcept;
    // NSString → UTF-8; nil и строка без представления UTF-8 дают пустую строку (ревью Т-49)
    // NSString → UTF-8; nil and a string without a UTF-8 form give an empty string (review T-49)
    std::string utf8(NSString* text) noexcept;

    /*
    ** Кодировщик команд, закрываемый при любом выходе из области — и при исключении (ревью Т-49): незакрытый
    ** кодировщик у брошенного командного буфера — ошибка слоя проверки Metal. Паттерн RAII; adopt закрывает
    ** текущий и берёт новый (проход интерфейса прерывается подложкой и начинается заново).
    **
    ** A command encoder closed on any scope exit — on an exception too (review T-49): an unclosed encoder of an
    ** abandoned command buffer is a Metal validation layer error. The RAII pattern; adopt closes the current one and
    ** takes a new one (the UI pass is interrupted by a backdrop and started again).
    */
    class EncoderScope
    {
    public:
        explicit EncoderScope(id<MTLCommandEncoder> encoder);
        ~EncoderScope();
        EncoderScope(const EncoderScope&) = delete;
        EncoderScope& operator=(const EncoderScope&) = delete;

        // Закрыть текущий кодировщик и взять новый / Close the current encoder and take a new one
        void adopt(id<MTLCommandEncoder> encoder);
        // Закрыть сейчас (дальше деструктору делать нечего) / Close now (nothing is left for the destructor)
        void end();

    private:
        id<MTLCommandEncoder> _encoder = nil;
    };

    /*
    ** Бэкенд Metal вне экрана или в окно (паттерн Bridge, как Dx11Backend: та же раскладка привязок и тот же порядок
    ** проходов размытия, чтобы кадры совпадали с эталонами D3D12 WARP с допуском).
    **
    ** The Metal backend, offscreen or into a window (the Bridge pattern, as Dx11Backend: the same binding layout and the same order
    ** of the blur passes, so that the frames match the D3D12 WARP baselines within a tolerance).
    */
    class MetalBackend final : public RenderBackend, public SnapshotSource
    {
    public:
        explicit MetalBackend(const BackendOptions& options);
        ~MetalBackend() override;

        void resize(std::uint32_t width, std::uint32_t height) override;
        std::uint32_t createTexture(const TextureDesc& desc) override;
        void updateTexture(std::uint32_t slot, const TextureRegion& region,
            const std::uint8_t* pixels, std::uint32_t rowPitch) override;
        void releaseTexture(std::uint32_t slot) override;
        FrameStatus render(const FrameData& frame) override;
        std::vector<std::string> takeDiagnostics() override;
        AdapterInfo adapterInfo() const override;
        std::string lostReason() const override;
        void readPixels(std::vector<std::uint8_t>& rgba, std::uint32_t& width, std::uint32_t& height) override;

    private:
        // Устройство, очередь, библиотека шейдеров и конвейеры (metal_device.mm)
        // The device, the queue, the shader library and the pipelines (metal_device.mm)
        void createDevice();
        void createPipelines();
        id<MTLRenderPipelineState> makePipeline(NSString* vertex, NSString* fragment, bool blend, const char* name);
        // Поверхность кадра и заглушка 1×1 для несвязанных текстур (metal_backend.mm)
        // The frame surface and a 1×1 stub for unbound textures (metal_backend.mm)
        void createSurface();
        // Ожидание последнего отправленного командного буфера (metal_readback.mm)
        // Waiting for the last submitted command buffer (metal_readback.mm)
        void waitIdle();
        // Окно: настройка слоя и показ кадра; present — false, если слой не дал drawable (metal_present.mm)
        // The window: setting up the layer and showing the frame; present — false if the layer gave no drawable (metal_present.mm)
        void attachLayer();
        bool present(id<MTLCommandBuffer> buffer);
        // Командный буфер кадра: загрузки, проходы, показ, отправка; true — кадр показан (metal_present.mm)
        // The frame command buffer: the uploads, the passes, showing, the submission; true — the frame is shown (metal_present.mm)
        bool record(const FrameData& frame);

        // Кадр: проверка, данные кадра, отправка (metal_frame.mm)
        // The frame: the check, the frame data, the submission (metal_frame.mm)
        void validate(const FrameData& frame) const;
        void uploadFrame(const FrameData& frame);
        void ensureCapacity(id<MTLBuffer> __strong& buffer, std::size_t bytes, const char* name);
        void submit(id<MTLCommandBuffer> buffer);

        // Загрузка текстур через промежуточный буфер (metal_textures.mm)
        // Texture uploads through a staging buffer (metal_textures.mm)
        void queueUpload(id<MTLTexture> texture, const TextureRegion& region, const std::uint8_t* pixels,
            std::uint32_t rowPitch, std::uint32_t bytes);
        void encodeUploads(id<MTLCommandBuffer> buffer);

        // Проходы отрисовки (metal_draw.mm) / The render passes (metal_draw.mm)
        void drawFrame(id<MTLCommandBuffer> buffer, const FrameData& frame);
        id<MTLRenderCommandEncoder> beginUi(id<MTLCommandBuffer> buffer, const FrameData& frame, bool clear);
        void drawSegment(id<MTLRenderCommandEncoder> encoder, const FrameData& frame, const FrameSegment& segment);
        void drawBatch(id<MTLRenderCommandEncoder> encoder, const FrameData& frame, std::uint32_t first,
            std::uint32_t count, std::uint32_t image);

        // Размытая подложка: копия кадра и двойной Kawase (metal_backdrop.mm)
        // The blurred backdrop: a frame copy and dual Kawase (metal_backdrop.mm)
        void backdrop(id<MTLCommandBuffer> buffer, float radius);
        void ensureBackdrop();
        void blur(id<MTLCommandBuffer> buffer, std::uint32_t source, std::uint32_t destination,
            id<MTLRenderPipelineState> pipeline);

        // Порядок полей задаёт порядок уничтожения (обратный объявлению): сначала последний командный буфер
        // и данные кадров, затем текстуры, конвейеры, очередь и последним — устройство. Деструктор перед этим
        // ждёт все кадры в полёте (waitIdle + семафор).
        // The field order sets the destruction order (reverse to the declaration): first the last command buffer
        // and the frame data, then the textures, the pipelines, the queue and the device last. The destructor
        // waits for all the frames in flight before that (waitIdle + the semaphore).
        BackendOptions _options;
        std::shared_ptr<SharedState> _shared;
        id<MTLDevice> _device = nil;
        id<MTLCommandQueue> _queue = nil;
        // Слой окна (nil — вне экрана) и формат поверхности и конвейеров: RGBA8 вне экрана, BGRA8 с окном
        // The window layer (nil — offscreen) and the surface and pipeline format: RGBA8 offscreen, BGRA8 with a window
        CAMetalLayer* _layer = nil;
        MTLPixelFormat _format = MTLPixelFormatRGBA8Unorm;
        id<MTLLibrary> _library = nil;
        id<MTLRenderPipelineState> _ui = nil;
        id<MTLRenderPipelineState> _down = nil;
        id<MTLRenderPipelineState> _up = nil;
        id<MTLSamplerState> _sampler = nil;
        Texture _surface;
        Texture _blur[BLUR_LEVELS];
        Texture _dummy;
        std::vector<Texture> _textures;
        std::vector<PendingUpload> _uploads;
        FrameBuffers _frames[FRAMES_IN_FLIGHT];
        std::uint32_t _frameIndex = 0;
        id<MTLCommandBuffer> _last = nil;
    };
}

// Проверка вызова Metal с NSError или условием / A Metal call check with an NSError or a condition
#define GRAN_METAL_CHECK(ok, error, call) ::gran::rhi::metal::check((ok), (error), (call), __FILE__, __LINE__)

#endif
