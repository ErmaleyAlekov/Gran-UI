#include "gran/rhi/vulkan/internal/swap_extent.hpp"
#include <algorithm>

namespace gran::rhi::vk
{
    // Нулевая поверхность ждёт восстановления окна / A zero-sized surface waits for window restoration
    bool chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps, VkExtent2D requested, VkExtent2D& extent) noexcept
    {
        extent = caps.currentExtent;
        if (extent.width == UINT32_MAX)
            extent = {std::clamp(requested.width, caps.minImageExtent.width, caps.maxImageExtent.width),
                std::clamp(requested.height, caps.minImageExtent.height, caps.maxImageExtent.height)};
        return extent.width != 0 && extent.height != 0;
    }
}
