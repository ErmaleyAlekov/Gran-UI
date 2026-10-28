#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <cstring>

namespace gran::rhi::vk
{
    // Отладочный слой необязателен: на целевой машине SDK может отсутствовать.
    // The validation layer is optional: the target machine may have no SDK.
    void VulkanBackend::createInstance()
    {
        auto app = structure<VkApplicationInfo>(VK_STRUCTURE_TYPE_APPLICATION_INFO);
        app.pApplicationName = "Gran UI";
        app.apiVersion = VK_API_VERSION_1_0;
        std::vector<const char*> extensions;
        if (_options.nativeWindow != nullptr)
            extensions = {VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_xlib_surface"};
        const char* validation = nullptr;
        instanceLayers(extensions, validation);
        auto info = structure<VkInstanceCreateInfo>(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
        info.pApplicationInfo = &app;
        info.enabledExtensionCount = static_cast<unsigned>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        if (validation != nullptr) { info.enabledLayerCount = 1; info.ppEnabledLayerNames = &validation; }
#if defined(VK_EXT_validation_features) && VK_HEADER_VERSION >= 162
        VkValidationFeatureEnableEXT enable = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
        auto features = structure<VkValidationFeaturesEXT>(VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT);
        features.enabledValidationFeatureCount = 1; features.pEnabledValidationFeatures = &enable;
        if (_syncValidation) info.pNext = &features;
#endif
        GRAN_CHECK_VK(vkCreateInstance(&info, nullptr, _instance.put(std::bind(vkDestroyInstance, std::placeholders::_1, nullptr))));
    }

    // Граница драйвера noexcept; даже нехватка памяти не пересекает callback.
    // The driver boundary is noexcept; even allocation failure cannot escape the callback.
    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanBackend::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
        VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user) noexcept
    {
        try
        {
            auto& backend = *static_cast<VulkanBackend*>(user);
            std::lock_guard<std::mutex> lock(backend._diagnosticMutex);
            backend._diagnostics.emplace_back(data->pMessage);
        }
        catch (...) { }
        return VK_FALSE;
    }

    // Журнал принимает предупреждения и ошибки; отсутствие слоя оставляет его пустым.
    // The log receives warnings and errors; without the layer it remains empty.
    void VulkanBackend::createDebug()
    {
        if (!_debugEnabled) return;
        auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_instance.get(), "vkCreateDebugUtilsMessengerEXT"));
        auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_instance.get(), "vkDestroyDebugUtilsMessengerEXT"));
        if (create == nullptr || destroy == nullptr) return;
        auto info = structure<VkDebugUtilsMessengerCreateInfoEXT>(VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = debugCallback;
        info.pUserData = this;
        GRAN_CHECK_VK(create(_instance.get(), &info, nullptr, _debug.put(std::bind(destroy, _instance.get(), std::placeholders::_1, nullptr))));
    }

    // Извлечение журнала атомарно для единственного потока рендера.
    // Extract the log for the single render thread.
    std::vector<std::string> VulkanBackend::takeDiagnostics()
    {
        std::vector<std::string> result;
        std::lock_guard<std::mutex> lock(_diagnosticMutex);
        result.swap(_diagnostics);
        return result;
    }
}
