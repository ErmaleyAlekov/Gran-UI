#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi::vk
{
    // Возможность ожидания и идентификатор показа текущего устройства.
    // Presentation wait availability and presentation ID of the current device.
    bool VulkanBackend::canConfirmPresentation() const
    {
        return _presentWait;
    }

    std::uint64_t VulkanBackend::lastPresentation() const
    {
        return _waitId;
    }
}
