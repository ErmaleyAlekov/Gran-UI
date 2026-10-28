#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <algorithm>

namespace gran::rhi::vk
{
    // Пересоздание после ожидания всей очереди; старые семафоры больше не используются presentation engine.
    // Recreate after waiting for the whole queue; old semaphores are no longer used by the presentation engine.
    void VulkanBackend::createSwapchain()
    {
        if (_surface.get() == VK_NULL_HANDLE) return;
        VkSurfaceCapabilitiesKHR caps{};
        GRAN_CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical, _surface.get(), &caps));
        if (!(caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) invalid("surface transfer");
        _swapExtent = caps.currentExtent;
        if (_swapExtent.width == UINT32_MAX)
            _swapExtent = {std::clamp(_options.width, caps.minImageExtent.width, caps.maxImageExtent.width),
                std::clamp(_options.height, caps.minImageExtent.height, caps.maxImageExtent.height)};
        unsigned count = caps.minImageCount + 1;
        if (caps.maxImageCount != 0) count = std::min(count, caps.maxImageCount);
        VkSurfaceFormatKHR format = surfaceFormat();
        auto info = structure<VkSwapchainCreateInfoKHR>(VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
        info.surface = _surface.get(); info.minImageCount = count;
        info.imageFormat = format.format; info.imageColorSpace = format.colorSpace; info.imageExtent = _swapExtent;
        info.imageArrayLayers = 1; info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.preTransform = caps.currentTransform; info.presentMode = presentMode(); info.clipped = VK_TRUE;
        const VkCompositeAlphaFlagBitsKHR modes[] = {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};
        for (auto mode : modes) if (caps.supportedCompositeAlpha & mode) { info.compositeAlpha = mode; break; }
        GRAN_CHECK_VK(vkCreateSwapchainKHR(_device.get(), &info, nullptr,
            _swapchain.put(std::bind(vkDestroySwapchainKHR, _device.get(), std::placeholders::_1, nullptr))));
        swapchainImages();
    }

    // Семафор finished принадлежит изображению: acquire гарантирует завершение прежнего present.
    // A finished semaphore belongs to its image: acquire guarantees completion of the previous present.
    void VulkanBackend::swapchainImages()
    {
        unsigned count = 0;
        GRAN_CHECK_VK(vkGetSwapchainImagesKHR(_device.get(), _swapchain.get(), &count, nullptr));
        _swapImages.resize(count);
        GRAN_CHECK_VK(vkGetSwapchainImagesKHR(_device.get(), _swapchain.get(), &count, _swapImages.data()));
        _presented.clear(); _presented.resize(count);
        auto info = structure<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
        for (auto& semaphore : _presented)
            GRAN_CHECK_VK(vkCreateSemaphore(_device.get(), &info, nullptr,
                semaphore.put(std::bind(vkDestroySemaphore, _device.get(), std::placeholders::_1, nullptr))));
    }

    // OUT_OF_DATE не оставляет сброшенный fence; SUBOPTIMAL завершает кадр и назначает пересоздание.
    // OUT_OF_DATE leaves no reset fence; SUBOPTIMAL completes the frame and schedules recreation.
    bool VulkanBackend::acquire()
    {
        if (_swapchain.get() == VK_NULL_HANDLE) return true;
        VkResult result = vkAcquireNextImageKHR(_device.get(), _swapchain.get(), UINT64_MAX,
            _frames[_current].acquired.get(), VK_NULL_HANDLE, &_swapIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) { _recreate = true; return false; }
        if (result == VK_SUBOPTIMAL_KHR) { _recreate = true; return true; }
        GRAN_CHECK_VK(result);
        return true;
    }

    // Нормальное изменение поверхности не считается потерей устройства.
    // A normal surface change is not treated as device loss.
    FrameStatus VulkanBackend::present()
    {
        if (_swapchain.get() == VK_NULL_HANDLE) return FrameStatus::PRESENTED;
        VkSemaphore finished = _presented[_swapIndex].get();
        VkSwapchainKHR swapchain = _swapchain.get();
        auto info = structure<VkPresentInfoKHR>(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
        info.waitSemaphoreCount = 1; info.pWaitSemaphores = &finished;
        info.swapchainCount = 1; info.pSwapchains = &swapchain; info.pImageIndices = &_swapIndex;
        VkResult result = vkQueuePresentKHR(_queue, &info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) _recreate = true;
        else GRAN_CHECK_VK(result);
        return FrameStatus::PRESENTED;
    }
}
