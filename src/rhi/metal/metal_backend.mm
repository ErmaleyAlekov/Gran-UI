#include "gran/rhi/metal/internal/metal_backend_impl.hpp"

#include <algorithm>

namespace gran::rhi::metal
{
    // Окна у Metal пока нет (CAMetalLayer — с платформой Cocoa, M6): только рисование вне экрана
    // Metal has no window yet (CAMetalLayer comes with the Cocoa platform, M6): offscreen drawing only
    MetalBackend::MetalBackend(const BackendOptions& options) : _options(options), _shared(std::make_shared<SharedState>())
    {
        if (options.nativeWindow != nullptr)
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_WINDOW, {"Metal"}), 0);
        _options.width = std::max(1u, options.width);
        _options.height = std::max(1u, options.height);
        _shared->inFlight = dispatch_semaphore_create(FRAMES_IN_FLIGHT);
        createDevice();
        createPipelines();
        createSurface();
    }

    // Порядок по gpu_rules.md: дождаться GPU, затем поля освобождаются в обратном порядке объявления.
    // Семафор забирается FRAMES_IN_FLIGHT раз — значит, все обработчики завершения уже отработали.
    // The order from gpu_rules.md: wait for the GPU, then the fields are released in reverse declaration order.
    // The semaphore is taken FRAMES_IN_FLIGHT times — so all the completion handlers have already run.
    MetalBackend::~MetalBackend()
    {
        try { waitIdle(); } catch (const GpuException&) {}
        for (std::uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
            dispatch_semaphore_wait(_shared->inFlight, DISPATCH_TIME_FOREVER);
        for (std::uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
            dispatch_semaphore_signal(_shared->inFlight);
        _uploads.clear();
    }

    // Новый размер: старые поверхность и уровни размытия освобождаются после кадров в полёте
    // (их удерживают командные буферы), поэтому ждать GPU не нужно
    // A new size: the old surface and blur levels are released after the frames in flight
    // (the command buffers retain them), so there is no need to wait for the GPU
    void MetalBackend::resize(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0 || (width == _options.width && height == _options.height))
            return;
        if (width > 16384 || height > 16384)
            invalid("surface");
        for (Texture& level : _blur)
            level = Texture{};
        _options.width = width;
        _options.height = height;
        createSurface();
    }

    // Поверхность кадра (цель + источник размытия) и заглушка 1×1: шейдер ссылается на все четыре текстуры,
    // а слой проверки Metal требует, чтобы каждая была привязана. Заглушка обнуляется первой загрузкой.
    // The frame surface (a target + the blur source) and a 1×1 stub: the shader refers to all four textures,
    // and the Metal validation layer requires each to be bound. The stub is zeroed by the first upload.
    void MetalBackend::createSurface()
    {
        makeTexture(_device, _surface, TextureDesc{_options.width, _options.height, TextureFormat::RGBA8,
            "gran.metal.surface"}, true);
        if (_dummy.resource != nil)
            return;
        makeTexture(_device, _dummy, TextureDesc{1, 1, TextureFormat::RGBA8, "gran.metal.dummy"}, false);
        queueUpload(_dummy.resource, TextureRegion{0, 0, 1, 1}, nullptr, 0, 4);
    }
}

namespace gran::rhi
{
    std::unique_ptr<RenderBackend> createMetalBackend(const BackendOptions& options)
    {
        return std::make_unique<metal::MetalBackend>(options);
    }
}
