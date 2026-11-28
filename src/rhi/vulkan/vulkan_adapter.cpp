#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

#include <string>
#include <cstring>

namespace gran::rhi::vk
{
    /*
    ** Имя и версия драйвера из свойств физического устройства. Кодировка driverVersion у производителей
    ** разная (у NVIDIA своя), поэтому выводится общая раскладка major.minor.patch и само число.
    **
    ** The name and the driver version from the physical device properties. The driverVersion encoding differs
    ** between vendors (NVIDIA has its own), so the common major.minor.patch layout and the number itself are printed.
    */
    AdapterInfo VulkanBackend::adapterInfo() const
    {
        AdapterInfo info;
        std::uint32_t version = _properties.driverVersion;

        info.api = "Vulkan";
        info.name = _properties.deviceName;
        info.driver = std::to_string(version >> 22) + "." + std::to_string((version >> 12) & 0x3FFu) + "."
            + std::to_string(version & 0xFFFu) + " (" + std::to_string(version) + ")";
        info.software = _properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
        return info;
    }

    // У Vulkan нет запроса причины: потерю сообщает только этот код / Vulkan has no reason query: only this code reports the loss
    std::string VulkanBackend::lostReason() const
    {
        return "VK_ERROR_DEVICE_LOST";
    }

    bool VulkanBackend::supportsMemoryBudget(VkPhysicalDevice candidate) const
    {
#ifdef VK_EXT_memory_budget
        if (_apiVersion < VK_API_VERSION_1_1) return false;
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(candidate, &props);
        if (props.apiVersion < VK_API_VERSION_1_1) return false;
        auto list = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
            vkGetInstanceProcAddr(_instance.get(), "vkEnumerateDeviceExtensionProperties"));
        unsigned count = 0;
        if (!list || list(candidate, nullptr, &count, nullptr) != VK_SUCCESS) return false;
        std::vector<VkExtensionProperties> extensions(count);
        if (list(candidate, nullptr, &count, extensions.data()) != VK_SUCCESS) return false;
        for (const auto& ext : extensions)
            if (std::strcmp(ext.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0) return true;
#else
        (void)candidate;
#endif
        return false;
    }

    VideoMemoryInfo VulkanBackend::videoMemoryInfo() const
    {
        VideoMemoryInfo info;
        for (const auto& image : _textures)
            if (image.handle.get()) info.textureBytes += std::uint64_t(image.width) * image.height * image.bytes;
        for (const auto& frame : _frames)
            info.bufferBytes += frame.upload.size + frame.instances.size + frame.clips.size;
#ifdef VK_EXT_memory_budget
        if (!_memoryBudget) return info;
        auto query = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2>(
            vkGetInstanceProcAddr(_instance.get(), "vkGetPhysicalDeviceMemoryProperties2"));
        if (!query) return info;
        auto budget = structure<VkPhysicalDeviceMemoryBudgetPropertiesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT);
        auto props = structure<VkPhysicalDeviceMemoryProperties2>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2);
        props.pNext = &budget;
        query(_physical, &props);
        for (unsigned i = 0; i < props.memoryProperties.memoryHeapCount; ++i)
            if (props.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                info.budgetAvailable = true;
                info.budgetBytes += budget.heapBudget[i];
                info.usageBytes += budget.heapUsage[i];
            }
#endif
        return info;
    }
}
