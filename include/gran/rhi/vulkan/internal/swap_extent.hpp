#ifndef GRAN_RHI_VULKAN_SWAP_EXTENT_HPP
#define GRAN_RHI_VULKAN_SWAP_EXTENT_HPP

#include "gran/rhi/vulkan/internal/vulkan_api.hpp"

namespace gran::rhi::vk
{
    // Нулевая поверхность ждёт восстановления окна / A zero-sized surface waits for window restoration
    bool chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps, VkExtent2D requested, VkExtent2D& extent) noexcept;
}

#endif
