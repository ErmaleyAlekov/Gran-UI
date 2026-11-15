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
*/
#if !defined(__OBJC__) || !__has_feature(objc_arc)
#error "metal_backend_impl.hpp: only for Objective-C++ with ARC (-fobjc-arc)"
#endif

#include "gran/rhi/metal/metal_backend.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#import <Metal/Metal.h>

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
    // Текстура RGBA8 или R8 в памяти GPU; target — ещё и цель отрисовки / An RGBA8 or R8 texture in GPU memory; target — also a render target
    void makeTexture(id<MTLDevice> device, Texture& texture, const TextureDesc& desc, bool target);
    // Итог командного буфера → SharedState; вызывается из обработчика завершения
    // A command buffer result → SharedState; called from the completion handler
    void completed(SharedState& state, id<MTLCommandBuffer> buffer) noexcept;

    /*
    ** Бэкенд Metal вне экрана (паттерн Bridge, как Dx11Backend: та же раскладка привязок и тот же порядок
    ** проходов размытия, чтобы кадры совпадали с эталонами D3D12 WARP с допуском).
    **
    ** The offscreen Metal backend (the Bridge pattern, as Dx11Backend: the same binding layout and the same order
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
