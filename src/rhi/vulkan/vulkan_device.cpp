#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <cstring>

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
        float priority = 1.0f;
        auto queue = structure<VkDeviceQueueCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
        queue.queueFamilyIndex = family;
        queue.queueCount = 1;
        queue.pQueuePriorities = &priority;
        auto info = structure<VkDeviceCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &queue;
        const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        if (_surface.get() != VK_NULL_HANDLE) { info.enabledExtensionCount = 1; info.ppEnabledExtensionNames = &extension; }
        VkResult result = vkCreateDevice(candidate, &info, nullptr,
            _device.put(std::bind(vkDestroyDevice, std::placeholders::_1, nullptr)));
        if (result != VK_SUCCESS) return false;
        _physical = candidate;
        _family = family;
        vkGetPhysicalDeviceProperties(candidate, &_properties);
        vkGetDeviceQueue(_device.get(), family, 0, &_queue);
        return true;
    }

    // CPU пробуется последним; forceWarp совместим с прежним API и требует именно CPU.
    // CPU is tried last; forceWarp preserves the existing API and requires a CPU device.
    void VulkanBackend::createDevice()
    {
        unsigned count = 0;
        GRAN_CHECK_VK(vkEnumeratePhysicalDevices(_instance.get(), &count, nullptr));
        std::vector<VkPhysicalDevice> devices(count);
        GRAN_CHECK_VK(vkEnumeratePhysicalDevices(_instance.get(), &count, devices.data()));
        const VkPhysicalDeviceType order[] = {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
            VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, VK_PHYSICAL_DEVICE_TYPE_OTHER, VK_PHYSICAL_DEVICE_TYPE_CPU};
        for (auto type : order)
        {
            if (_options.forceWarp && type != VK_PHYSICAL_DEVICE_TYPE_CPU) continue;
            for (auto device : devices)
            {
                VkPhysicalDeviceProperties properties{};
                vkGetPhysicalDeviceProperties(device, &properties);
                if (properties.deviceType == type && tryDevice(device)) return;
            }
        }
        throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_NO_ADAPTER, {"Vulkan 1.0 / Mesa lavapipe"}), 0);
    }

    // Тип памяти берётся из требований ресурса и свойств выбранного устройства.
    // Choose a memory type from resource requirements and the selected device properties.
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
