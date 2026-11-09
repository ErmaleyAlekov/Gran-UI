#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <cstring>

/*
** Ожидание показа кадра (VK_KHR_present_id + VK_KHR_present_wait). Нужно, пока край окна тянут мышью в X11:
** окно отвечает оконному менеджеру «кадр нового размера готов» (_NET_WM_SYNC_REQUEST), и ответ должен прийти,
** когда кадр уже в окне. vkQueuePresentKHR только ставит кадр в очередь (FIFO — до следующего обновления экрана).
** Замер на стенде (Т-24, три прогона попеременно): с ожиданием худший ответ менеджеру на Intel — 145 мс, без
** него — больше 200 мс (окно «залипает»); в среднем разница в пределах 6 мс (NVIDIA 42 против 37, Intel 87 против 90).
** Расширения требуют Vulkan 1.1; нет их — бэкенд работает без ожидания.
**
** Waiting for a frame to be shown (VK_KHR_present_id + VK_KHR_present_wait). Needed while the window edge is dragged
** with the mouse on X11: the window answers the window manager "the frame of the new size is ready"
** (_NET_WM_SYNC_REQUEST), and the answer must come when the frame is already in the window. vkQueuePresentKHR only
** queues the frame (FIFO — until the next screen refresh). Measured on the stand (T-24, three alternating runs):
** with waiting the worst answer to the manager on Intel is 145 ms, without it — over 200 ms (the window "sticks");
** on average the difference is within 6 ms (NVIDIA 42 vs 37, Intel 87 vs 90). The extensions need Vulkan 1.1;
** without them the backend works without waiting.
*/
namespace gran::rhi::vk
{
    namespace
    {
        // Дольше кадр показа не ждём: окно не должно зависать из-за драйвера (100 мс)
        // A frame is not waited for longer: the window must not hang because of the driver (100 ms)
        constexpr std::uint64_t PRESENT_WAIT_TIMEOUT = 100000000;
    }

    // Vulkan 1.1, если его знает загрузчик (vkEnumerateInstanceVersion есть только с 1.1), иначе 1.0
    // Vulkan 1.1 if the loader knows it (vkEnumerateInstanceVersion exists only since 1.1), otherwise 1.0
    std::uint32_t VulkanBackend::instanceVersion() const
    {
        auto enumerate = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
            vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
        std::uint32_t version = VK_API_VERSION_1_0;

        if (enumerate == nullptr || enumerate(&version) != VK_SUCCESS || version < VK_API_VERSION_1_1)
            return VK_API_VERSION_1_0;
        return VK_API_VERSION_1_1;
    }

    // Оба расширения в списке устройства / Both extensions are in the device list
    bool VulkanBackend::hasPresentWait(VkPhysicalDevice candidate) const
    {
        auto list = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
            vkGetInstanceProcAddr(_instance.get(), "vkEnumerateDeviceExtensionProperties"));
        unsigned count = 0;
        int found = 0;

        if (list == nullptr || list(candidate, nullptr, &count, nullptr) != VK_SUCCESS)
            return false;
        std::vector<VkExtensionProperties> extensions(count);
        if (list(candidate, nullptr, &count, extensions.data()) != VK_SUCCESS)
            return false;
        for (const VkExtensionProperties& extension : extensions)
        {
            if (std::strcmp(extension.extensionName, "VK_KHR_present_id") == 0
                || std::strcmp(extension.extensionName, "VK_KHR_present_wait") == 0)
                ++found;
        }
        return found == 2;
    }

    // Окно, Vulkan 1.1 у экземпляра и устройства, расширения и обе возможности (features)
    // A window, Vulkan 1.1 for the instance and the device, the extensions and both features
    bool VulkanBackend::presentWaitSupported(VkPhysicalDevice candidate) const
    {
#if defined(VK_KHR_present_wait) && defined(VK_KHR_present_id)
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        if (_surface.get() == VK_NULL_HANDLE || _apiVersion < VK_API_VERSION_1_1
            || properties.apiVersion < VK_API_VERSION_1_1 || !hasPresentWait(candidate))
            return false;
        auto query = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
            vkGetInstanceProcAddr(_instance.get(), "vkGetPhysicalDeviceFeatures2"));
        if (query == nullptr)
            return false;
        auto wait = structure<VkPhysicalDevicePresentWaitFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR);
        auto id = structure<VkPhysicalDevicePresentIdFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR);
        auto features = structure<VkPhysicalDeviceFeatures2>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2);
        id.pNext = &wait;
        features.pNext = &id;
        query(candidate, &features);
        return id.presentId == VK_TRUE && wait.presentWait == VK_TRUE;
#else
        (void)candidate;
        return false;
#endif
    }

    /*
    ** Ждёт, пока последний показанный кадр окажется в окне (не дольше PRESENT_WAIT_TIMEOUT). Таймаут, устаревшая
    ** цепочка и потеря поверхности — не ошибка: ответ менеджеру уйдёт как раньше, а потерю устройства увидит
    ** следующий кадр.
    **
    ** Waits until the last presented frame is in the window (not longer than PRESENT_WAIT_TIMEOUT). A timeout, an
    ** out-of-date chain and a lost surface are not errors: the answer to the manager goes as before, and the next
    ** frame sees a device loss.
    */
    void VulkanBackend::waitPresented()
    {
#if defined(VK_KHR_present_wait)
        if (!_presentWait || _waitId == 0 || _swapchain.get() == VK_NULL_HANDLE)
            return;
        auto wait = reinterpret_cast<PFN_vkWaitForPresentKHR>(_waitForPresent);
        VkResult result = wait(_device.get(), _swapchain.get(), _waitId, PRESENT_WAIT_TIMEOUT);
        _waitId = 0;
        if (result == VK_TIMEOUT || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR
            || result == VK_ERROR_SURFACE_LOST_KHR || result == VK_ERROR_DEVICE_LOST)
            return;
        GRAN_CHECK_VK(result);
#endif
    }
}
