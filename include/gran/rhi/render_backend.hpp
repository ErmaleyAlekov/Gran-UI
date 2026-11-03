#ifndef GRAN_RHI_RENDER_BACKEND_HPP
#define GRAN_RHI_RENDER_BACKEND_HPP

#include "gran/render/gpu_types.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gran::rhi
{
    // Формат текстуры: R8 — покрытие глифов, RGBA8 — картинки и рампы (альфа предумножена)
    // A texture format: R8 — the glyph coverage, RGBA8 — images and ramps (premultiplied alpha)
    enum class TextureFormat : std::uint8_t
    {
        R8,
        RGBA8
    };

    // Описание текстуры / A texture description
    struct TextureDesc
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        TextureFormat format = TextureFormat::RGBA8;
        const char* debugName = "texture";
    };

    // Прямоугольник обновления текстуры в текселях / A texture update rectangle in texels
    struct TextureRegion
    {
        std::uint32_t x = 0;
        std::uint32_t y = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };

    /*
    ** Сегмент кадра: экземпляры [first, first + count) одним вызовом. backdropBefore —
    ** перед сегментом снять копию кадра и размыть её с силой backdropRadius (пиксели) для подложек.
    **
    ** A frame segment: the instances [first, first + count) in one call. backdropBefore —
    ** before the segment take a copy of the frame and blur it with backdropRadius (pixels) for backdrops.
    */
    struct FrameSegment
    {
        std::uint32_t first = 0;
        std::uint32_t count = 0;
        bool backdropBefore = false;
        float backdropRadius = 0.0f;
    };

    // Данные кадра (без владения: живут до возврата из render) / The frame data (not owned: they live until render returns)
    struct FrameData
    {
        const render::GpuInstance* instances = nullptr;
        std::uint32_t instanceCount = 0;
        const render::GpuClip* clips = nullptr;
        std::uint32_t clipCount = 0;
        const FrameSegment* segments = nullptr;
        std::uint32_t segmentCount = 0;
        float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        std::uint32_t rampRows = 1;
        std::uint32_t atlasSlot = 0;
        std::uint32_t rampSlot = 0;
    };

    // Итог кадра / The frame result
    // Сведения о видеокарте: API, имя, версия драйвера, программный ли адаптер (WARP, lavapipe) / The video card information: the API, the name, the driver version, whether the adapter is software (WARP, lavapipe)
    struct AdapterInfo
    {
        std::string api;
        std::string name;
        std::string driver;
        bool software = false;
    };

    enum class FrameStatus : std::uint8_t
    {
        PRESENTED,      // кадр показан / the frame is shown
        OCCLUDED,       // окно скрыто, кадр пропущен / the window is hidden, the frame is skipped
        DEVICE_LOST     // устройство потеряно: пересоздать бэкенд / the device is lost: recreate the backend
    };

    /*
    ** Бэкенд отрисовки (паттерн Bridge: рендерер не знает, D3D12 это, D3D11, Vulkan или запись
    ** для тестов). Все методы вызываются из одного потока рендера. Индексы текстур — логические
    ** слоты: D3D12 использует таблицу дескрипторов, D3D11 привязывает слот при смене пакета.
    **
    ** A render backend (the Bridge pattern: the renderer does not know whether it is D3D12, D3D11,
    ** Vulkan or a recorder for tests). All methods are called from one render thread. Texture indices
    ** are logical slots: D3D12 uses a descriptor table, D3D11 binds a slot when a batch changes.
    */
    class RenderBackend
    {
    public:
        virtual ~RenderBackend() = default;

        // Новый размер цели (пиксели устройства) / A new target size (device pixels)
        virtual void resize(std::uint32_t width, std::uint32_t height) = 0;

        // Текстура в первом свободном слоте; возвращает слот / A texture in the first free slot; returns the slot
        virtual std::uint32_t createTexture(const TextureDesc& desc) = 0;

        // Обновление части текстуры; данные копируются сразу, на GPU попадут со следующим кадром
        // Updating a part of a texture; the data is copied at once and reaches the GPU with the next frame
        virtual void updateTexture(std::uint32_t slot, const TextureRegion& region, const std::uint8_t* pixels,
            std::uint32_t rowPitch) = 0;

        // Освобождение слота (фактически — когда GPU закончит кадры с ним) / Releasing a slot (actually — when the GPU finishes the frames using it)
        virtual void releaseTexture(std::uint32_t slot) = 0;

        // Запись и показ кадра / Recording and presenting a frame
        virtual FrameStatus render(const FrameData& frame) = 0;

        // Накопленные сообщения отладочного слоя (и их очистка); без слоя — пусто
        // The accumulated debug layer messages (and clearing them); without the layer — empty
        virtual std::vector<std::string> takeDiagnostics() = 0;

        // ---- render_backend.cpp ----

        // Видеокарта и драйвер для журнала; по умолчанию — пусто / The video card and the driver for the log; empty by default
        virtual AdapterInfo adapterInfo() const;
    };

    /*
    ** Снимок кадра для проверок без окна: RGBA8 построчно, сверху вниз, без выравнивания.
    ** Реализуют бэкенды, созданные без окна (offscreen).
    **
    ** A frame snapshot for checks without a window: RGBA8 row by row, top to bottom, unaligned.
    ** Implemented by the backends created without a window (offscreen).
    */
    class SnapshotSource
    {
    public:
        virtual ~SnapshotSource() = default;

        // Пиксели последнего кадра (ждёт GPU) / The pixels of the last frame (waits for the GPU)
        virtual void readPixels(std::vector<std::uint8_t>& rgba, std::uint32_t& width, std::uint32_t& height) = 0;
    };

    // Выбор API; AUTO выбирает API ОС / API selection; AUTO selects the platform API
    enum class BackendKind : std::uint8_t { AUTO, D3D11, D3D12, VULKAN };

    // Параметры создания бэкенда / The backend creation parameters
    struct BackendOptions
    {
        void* nativeWindow = nullptr;   // HWND, Window (X11) и т. п.; nullptr — без окна / HWND, Window (X11) etc.; nullptr — offscreen
        void* nativeDisplay = nullptr;  // Display* для X11 / Display* for X11
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        bool vsync = true;
        bool debugLayer = false;        // отладочный слой D3D12 / Vulkan / the D3D12 / Vulkan debug layer
        BackendKind backend = BackendKind::AUTO;
        bool forceWarp = false;         // CPU: WARP или Mesa lavapipe / CPU: WARP or Mesa lavapipe
    };
}

#endif
