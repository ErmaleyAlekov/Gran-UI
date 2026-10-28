#include "gran/rhi/backend_factory.hpp"
#include "gran/rhi/vulkan/vulkan_backend.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::rhi
{
    // На Linux AUTO и VULKAN используют Vulkan; запрос Direct3D явно отклоняется.
    // On Linux AUTO and VULKAN use Vulkan; an explicit Direct3D request is rejected.
    std::unique_ptr<RenderBackend> createPlatformBackend(const BackendOptions& options)
    {
        if (options.backend != BackendKind::AUTO && options.backend != BackendKind::VULKAN)
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_ADAPTER, {"Direct3D (Linux)"}), 0);
        try { return createVulkanBackend(options); }
        catch (const GpuException&)
        {
            if (options.forceWarp) throw;
            BackendOptions software = options;
            software.forceWarp = true;
            return createVulkanBackend(software);
        }
    }
}
