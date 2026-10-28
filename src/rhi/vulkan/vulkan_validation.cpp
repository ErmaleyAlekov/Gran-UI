#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <cstring>

namespace gran::rhi::vk
{
    // Слой и его расширения проверяются отдельно: отсутствие SDK не ломает запуск программы.
    // Check the layer and its extensions separately: absence of an SDK cannot break application startup.
    void VulkanBackend::instanceLayers(std::vector<const char*>& extensions, const char*& validation)
    {
        if (!_options.debugLayer) return;
        unsigned count = 0;
        GRAN_CHECK_VK(vkEnumerateInstanceLayerProperties(&count, nullptr));
        std::vector<VkLayerProperties> layers(count);
        GRAN_CHECK_VK(vkEnumerateInstanceLayerProperties(&count, layers.data()));
        for (const auto& layer : layers)
            if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) validation = "VK_LAYER_KHRONOS_validation";
        if (validation == nullptr) return;
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        _debugEnabled = true;
#if defined(VK_EXT_validation_features) && VK_HEADER_VERSION >= 162
        GRAN_CHECK_VK(vkEnumerateInstanceExtensionProperties(validation, &count, nullptr));
        std::vector<VkExtensionProperties> supported(count);
        GRAN_CHECK_VK(vkEnumerateInstanceExtensionProperties(validation, &count, supported.data()));
        for (const auto& extension : supported)
            if (std::strcmp(extension.extensionName, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME) == 0)
            { extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME); _syncValidation = true; }
#endif
    }
}
