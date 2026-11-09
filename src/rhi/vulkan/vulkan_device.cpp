#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include "gran/rhi/adapter_choice.hpp"
#include <cstring>
#include <string>

namespace gran::rhi::vk
{
    // Одна очередь должна уметь и рисовать, и показывать: владение изображениями не передаётся.
    // One queue must support both graphics and presentation: images need no ownership transfers.
    bool VulkanBackend::queueFamily(VkPhysicalDevice candidate, unsigned& family)
    {
        unsigned count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, families.data());
        for (family = 0; family < count; ++family)
        {
            VkBool32 present = VK_TRUE;
            if (_surface.get() != VK_NULL_HANDLE)
                GRAN_CHECK_VK(vkGetPhysicalDeviceSurfaceSupportKHR(candidate, family, _surface.get(), &present));
            if (families[family].queueCount != 0 && (families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present)
                return true;
        }
        return false;
    }

    // Ошибка создания одного GPU не мешает попробовать другой, включая lavapipe CPU.
    // Failure to create one GPU does not prevent trying another, including lavapipe CPU.
    bool VulkanBackend::tryDevice(VkPhysicalDevice candidate)
    {
        unsigned family = 0;
        if (!queueFamily(candidate, family)) return false;
        // Сначала с ожиданием показа; не создалось — без него (возможность есть в списке, но драйвер отказал)
        // First with presentation waiting; if that fails — without it (the feature is listed, but the driver refused)
        bool wait = presentWaitSupported(candidate);
        if (!createLogical(candidate, family, wait) && (!wait || !createLogical(candidate, family, false)))
            return false;
        _physical = candidate;
        _family = family;
        vkGetPhysicalDeviceProperties(candidate, &_properties);
        vkGetDeviceQueue(_device.get(), family, 0, &_queue);
        return true;
    }

    /*
    ** CPU пробуется последним; forceWarp совместим с прежним API и требует именно CPU.
    ** GRAN_ADAPTER (без forceWarp) оставляет только устройства с подходящим именем; нет таких — ошибка.
    **
    ** CPU is tried last; forceWarp preserves the existing API and requires a CPU device.
    ** GRAN_ADAPTER (without forceWarp) keeps only the devices with a matching name; none — an error.
    */
    void VulkanBackend::createDevice()
    {
        unsigned count = 0;
        GRAN_CHECK_VK(vkEnumeratePhysicalDevices(_instance.get(), &count, nullptr));
        std::vector<VkPhysicalDevice> devices(count);
        GRAN_CHECK_VK(vkEnumeratePhysicalDevices(_instance.get(), &count, devices.data()));
        const VkPhysicalDeviceType order[] = {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
            VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, VK_PHYSICAL_DEVICE_TYPE_OTHER, VK_PHYSICAL_DEVICE_TYPE_CPU};
        std::string seen;
        for (auto type : order)
        {
            if (_options.forceWarp && type != VK_PHYSICAL_DEVICE_TYPE_CPU) continue;
            for (auto device : devices)
            {
                VkPhysicalDeviceProperties properties{};
                vkGetPhysicalDeviceProperties(device, &properties);
                if (properties.deviceType != type) continue;
                seen += std::string(properties.deviceName) + "; ";
                if ((_options.forceWarp || adapterAllowed(properties.deviceName)) && tryDevice(device)) return;
            }
        }
        if (!_options.forceWarp && !requestedAdapter().empty()) adapterNotFound(seen);
        throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_ADAPTER, {"Vulkan 1.0 / Mesa lavapipe"}), 0);
    }

    // Тип памяти берётся из требований ресурса и свойств выбранного устройства.
    // Choose a memory type from resource requirements and the selected device properties.
    /*
    ** Логическое устройство с одной очередью; у окна — цепочка обмена и, если presentWait, ожидание показа
    ** (VK_KHR_present_id + VK_KHR_present_wait с включёнными возможностями, vulkan_present_wait.cpp).
    **
    ** A logical device with one queue; for a window — the swap chain and, if presentWait, presentation waiting
    ** (VK_KHR_present_id + VK_KHR_present_wait with the features enabled, vulkan_present_wait.cpp).
    */
    bool VulkanBackend::createLogical(VkPhysicalDevice candidate, unsigned family, bool presentWait)
    {
        float priority = 1.0f;
        auto queue = structure<VkDeviceQueueCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
        queue.queueFamilyIndex = family; queue.queueCount = 1; queue.pQueuePriorities = &priority;
        auto info = structure<VkDeviceCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
        info.queueCreateInfoCount = 1; info.pQueueCreateInfos = &queue;
        std::vector<const char*> extensions;
        if (_surface.get() != VK_NULL_HANDLE) extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#if defined(VK_KHR_present_wait) && defined(VK_KHR_present_id)
        auto wait = structure<VkPhysicalDevicePresentWaitFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR);
        auto id = structure<VkPhysicalDevicePresentIdFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR);
        wait.presentWait = VK_TRUE; id.presentId = VK_TRUE; id.pNext = &wait;
        if (presentWait) { extensions.push_back("VK_KHR_present_id"); extensions.push_back("VK_KHR_present_wait"); info.pNext = &id; }
#endif
        info.enabledExtensionCount = static_cast<unsigned>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        if (vkCreateDevice(candidate, &info, nullptr, _device.put(std::bind(vkDestroyDevice, std::placeholders::_1, nullptr)))
            != VK_SUCCESS) return false;
        _waitForPresent = nullptr;
        if (presentWait) _waitForPresent = vkGetDeviceProcAddr(_device.get(), "vkWaitForPresentKHR");
        _presentWait = _waitForPresent != nullptr;
        return true;
    }

    unsigned VulkanBackend::memoryType(unsigned bits, VkMemoryPropertyFlags flags)
    {
        VkPhysicalDeviceMemoryProperties properties{};
        vkGetPhysicalDeviceMemoryProperties(_physical, &properties);
        for (unsigned index = 0; index < properties.memoryTypeCount; ++index)
            if ((bits & (1u << index)) && (properties.memoryTypes[index].propertyFlags & flags) == flags)
                return index;
        invalid("memoryType");
        return 0;
    }
}
