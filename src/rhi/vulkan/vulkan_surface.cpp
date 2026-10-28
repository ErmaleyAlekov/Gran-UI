#define VK_USE_PLATFORM_XLIB_KHR
#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi::vk
{
    // Xlib surface не создаётся при offscreen: снимок работает и без DISPLAY.
    // No Xlib surface is created offscreen: snapshots also work without DISPLAY.
    void VulkanBackend::createSurface()
    {
        if (_options.nativeWindow == nullptr) return;
        if (_options.nativeDisplay == nullptr) invalid("Display*");
        auto info = structure<VkXlibSurfaceCreateInfoKHR>(VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR);
        info.dpy = static_cast<Display*>(_options.nativeDisplay);
        info.window = reinterpret_cast<std::uintptr_t>(_options.nativeWindow);
        GRAN_CHECK_VK(vkCreateXlibSurfaceKHR(_instance.get(), &info, nullptr,
            _surface.put(std::bind(vkDestroySurfaceKHR, _instance.get(), std::placeholders::_1, nullptr))));
    }

    // Предпочитаем UNORM: цвета CSS уже sRGB и не должны кодироваться повторно.
    // Prefer UNORM: CSS values are already sRGB and must not be encoded again.
    VkSurfaceFormatKHR VulkanBackend::surfaceFormat()
    {
        unsigned count = 0;
        GRAN_CHECK_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(_physical, _surface.get(), &count, nullptr));
        std::vector<VkSurfaceFormatKHR> formats(count);
        GRAN_CHECK_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(_physical, _surface.get(), &count, formats.data()));
        for (const auto& format : formats)
            if ((format.format == VK_FORMAT_B8G8R8A8_UNORM || format.format == VK_FORMAT_R8G8B8A8_UNORM)
                && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return format;
        if (count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
            return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        invalid("surface UNORM");
        return {};
    }

    // FIFO обязателен по спецификации; при отключённом vsync выбираем MAILBOX или IMMEDIATE.
    // FIFO is required by the specification; without vsync choose MAILBOX or IMMEDIATE.
    VkPresentModeKHR VulkanBackend::presentMode()
    {
        if (_options.vsync) return VK_PRESENT_MODE_FIFO_KHR;
        unsigned count = 0;
        GRAN_CHECK_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(_physical, _surface.get(), &count, nullptr));
        std::vector<VkPresentModeKHR> modes(count);
        GRAN_CHECK_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(_physical, _surface.get(), &count, modes.data()));
        for (auto mode : modes) if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
        for (auto mode : modes) if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) return mode;
        return VK_PRESENT_MODE_FIFO_KHR;
    }
}
