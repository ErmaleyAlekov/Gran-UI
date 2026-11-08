#include "gran/rhi/backend_factory.hpp"
#include "gran/rhi/vulkan/vulkan_backend.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/rhi/adapter_choice.hpp"

namespace gran::rhi
{
    // На Linux AUTO и VULKAN используют Vulkan; запрос Direct3D явно отклоняется. Отказ карты — переход на
    // lavapipe, но не при GRAN_ADAPTER: выбранная карта не подменяется программной (тест шёл бы не на ней).
    // On Linux AUTO and VULKAN use Vulkan; an explicit Direct3D request is rejected. A card failure falls back to
    // lavapipe, but not with GRAN_ADAPTER: the chosen card is not replaced by software (a test would run elsewhere).
    std::unique_ptr<RenderBackend> createPlatformBackend(const BackendOptions& options)
    {
        if (options.backend != BackendKind::AUTO && options.backend != BackendKind::VULKAN)
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_ADAPTER, {"Direct3D (Linux)"}), 0);
        try { return createVulkanBackend(options); }
        catch (const GpuException&)
        {
            if (options.forceWarp || !requestedAdapter().empty()) throw;
            BackendOptions software = options;
            software.forceWarp = true;
            return createVulkanBackend(software);
        }
    }
}
