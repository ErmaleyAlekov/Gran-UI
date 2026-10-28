#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"

#include <algorithm>

namespace gran::rhi::dx11
{
    // Ресурсы создаются в одном потоке, поверхность — после конвейера.
    // Resources are created on one thread, the surface follows the pipeline.
    Dx11Backend::Dx11Backend(const BackendOptions& options) : _options(options)
    {
        _options.width = std::max(1u, options.width);
        _options.height = std::max(1u, options.height);
        createDevice();
        createPipeline();
        createSurface();
    }

    // Деструктор не выпускает исключений даже после потери устройства.
    // The destructor never propagates exceptions, including after device loss.
    Dx11Backend::~Dx11Backend()
    {
        try { waitIdle(); } catch (const GpuException&) {}
        _context->ClearState();
        _context->Flush();
    }

    // Освобождаем все ссылки контекста на старые буферы swapchain до ResizeBuffers.
    // Release all context references to old swapchain buffers before ResizeBuffers.
    void Dx11Backend::resize(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0 || (width == _options.width && height == _options.height))
            return;
        if (width > 16384 || height > 16384)
            invalid("surface");
        waitIdle();
        _context->ClearState();
        _surface = Texture{};
        for (Texture& level : _blur)
            level = Texture{};
        _options.width = width;
        _options.height = height;
        if (_swapchain)
            GRAN_DX11_CHECK(_swapchain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
        createSurface();
    }

    // EVENT query используется только там, где необходим полный простой.
    // An EVENT query is used only where full idle is necessary.
    void Dx11Backend::waitIdle()
    {
        _context->End(_idle.get());
        _context->Flush();
        HRESULT result = S_FALSE;
        while (result == S_FALSE)
        {
            result = _context->GetData(_idle.get(), nullptr, 0, 0);
            if (result == S_FALSE)
            {
                GRAN_DX11_CHECK(_device->GetDeviceRemovedReason());
                Sleep(0);
            }
        }
        GRAN_DX11_CHECK(result);
    }
}

namespace gran::rhi
{
    // Общий интерфейс сохраняет доступ к SnapshotSource через dynamic_cast.
    // The shared interface preserves access to SnapshotSource through dynamic_cast.
    std::unique_ptr<RenderBackend> createDx11Backend(const BackendOptions& options)
    {
        return std::make_unique<dx11::Dx11Backend>(options);
    }
}
