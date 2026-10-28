#include "gran/rhi/backend_factory.hpp"
#include "gran/rhi/dx12/dx12_backend.hpp"
#include "gran/rhi/dx11/dx11_backend.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::rhi
{
    // На Windows бэкенд по умолчанию — Direct3D 12 / On Windows the default backend is Direct3D 12
    std::unique_ptr<RenderBackend> createPlatformBackend(const BackendOptions& options)
    {
        if (options.backend == BackendKind::VULKAN)
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_ADAPTER, {"Vulkan (Windows)"}), 0);
        if (options.backend == BackendKind::D3D11)
            return createDx11Backend(options);
        if (options.backend == BackendKind::D3D12)
            return createDx12Backend(options);
        try
        {
            return createDx12Backend(options);
        }
        catch (const GpuException&)
        {
            return createDx11Backend(options);
        }
    }
}
