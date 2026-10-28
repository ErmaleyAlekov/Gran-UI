#ifndef GRAN_RHI_VULKAN_API_HPP
#define GRAN_RHI_VULKAN_API_HPP

/*
** Vulkan без компоновки с libvulkan: заголовки дают только типы (VK_NO_PROTOTYPES), а функции —
** указатели, которые vk::loadVulkan заполняет из системного загрузчика libvulkan.so.1 через dlopen.
** Так программа запускается и без пакета Vulkan, а его отсутствие — понятная ошибка из каталога
** сообщений, а не отказ динамического загрузчика ОС до main.
**
** Указатели объявлены в пространстве gran::rhi::vk, а не глобально (как в volk): глобальные имена
** vkCreateInstance перехватили бы вызовы Vulkan самой программы, если она компонуется с libvulkan.
** Код бэкенда лежит в том же пространстве и вызывает vkCreateInstance(...) без изменений.
**
** Vulkan without linking libvulkan: the headers give only the types (VK_NO_PROTOTYPES), and the functions are
** pointers that vk::loadVulkan fills from the system loader libvulkan.so.1 through dlopen.
** So the program starts even without the Vulkan package, and its absence is a clear error from the message
** catalog rather than a refusal of the OS dynamic loader before main.
**
** The pointers are declared in the gran::rhi::vk namespace, not globally (as in volk): global vkCreateInstance
** names would intercept the program's own Vulkan calls if it links with libvulkan.
** The backend code lives in the same namespace and calls vkCreateInstance(...) unchanged.
*/
#ifndef VK_NO_PROTOTYPES
# define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

// Функции ядра Vulkan 1.0 и WSI, которые экспортирует любой загрузчик / The Vulkan 1.0 core and WSI functions any loader exports
#define GRAN_VK_FUNCTIONS(X) \
    X(vkGetInstanceProcAddr) X(vkGetDeviceProcAddr) X(vkCreateInstance) X(vkDestroyInstance) \
    X(vkEnumerateInstanceExtensionProperties) X(vkEnumerateInstanceLayerProperties) X(vkEnumeratePhysicalDevices) \
    X(vkGetPhysicalDeviceProperties) X(vkGetPhysicalDeviceMemoryProperties) X(vkGetPhysicalDeviceQueueFamilyProperties) \
    X(vkCreateDevice) X(vkDestroyDevice) X(vkGetDeviceQueue) X(vkDeviceWaitIdle) X(vkQueueSubmit) X(vkQueueWaitIdle) \
    X(vkAllocateMemory) X(vkFreeMemory) X(vkMapMemory) X(vkUnmapMemory) X(vkBindBufferMemory) X(vkBindImageMemory) \
    X(vkGetBufferMemoryRequirements) X(vkGetImageMemoryRequirements) X(vkCreateBuffer) X(vkDestroyBuffer) \
    X(vkCreateImage) X(vkDestroyImage) X(vkCreateImageView) X(vkDestroyImageView) X(vkCreateSampler) \
    X(vkDestroySampler) X(vkCreateShaderModule) X(vkDestroyShaderModule) X(vkCreateGraphicsPipelines) \
    X(vkDestroyPipeline) X(vkCreatePipelineLayout) X(vkDestroyPipelineLayout) X(vkCreateDescriptorSetLayout) \
    X(vkDestroyDescriptorSetLayout) X(vkCreateDescriptorPool) X(vkDestroyDescriptorPool) X(vkResetDescriptorPool) \
    X(vkAllocateDescriptorSets) X(vkUpdateDescriptorSets) X(vkCreateRenderPass) X(vkDestroyRenderPass) \
    X(vkCreateFramebuffer) X(vkDestroyFramebuffer) X(vkCreateCommandPool) X(vkDestroyCommandPool) \
    X(vkResetCommandPool) X(vkAllocateCommandBuffers) X(vkBeginCommandBuffer) X(vkEndCommandBuffer) \
    X(vkCreateFence) X(vkDestroyFence) X(vkResetFences) X(vkWaitForFences) X(vkCreateSemaphore) X(vkDestroySemaphore) \
    X(vkCmdBeginRenderPass) X(vkCmdEndRenderPass) X(vkCmdBindPipeline) X(vkCmdBindDescriptorSets) \
    X(vkCmdPushConstants) X(vkCmdSetViewport) X(vkCmdSetScissor) X(vkCmdDraw) X(vkCmdPipelineBarrier) \
    X(vkCmdClearAttachments) X(vkCmdClearColorImage) X(vkCmdCopyBufferToImage) X(vkCmdCopyImage) \
    X(vkCmdCopyImageToBuffer) X(vkCmdBlitImage) X(vkDestroySurfaceKHR) X(vkGetPhysicalDeviceSurfaceSupportKHR) \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    X(vkGetPhysicalDeviceSurfacePresentModesKHR) X(vkCreateSwapchainKHR) X(vkDestroySwapchainKHR) \
    X(vkGetSwapchainImagesKHR) X(vkAcquireNextImageKHR) X(vkQueuePresentKHR)

#define GRAN_VK_DECLARE(name) extern PFN_##name name;

namespace gran::rhi::vk
{
    GRAN_VK_FUNCTIONS(GRAN_VK_DECLARE)

#ifdef VK_USE_PLATFORM_XLIB_KHR
    // Поверхность Xlib — только там, где подключены заголовки X11 / The Xlib surface — only where the X11 headers are included
    extern PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
#endif

    /*
    ** Открыть libvulkan.so.1 и заполнить указатели (один раз на процесс, потокобезопасно; библиотека
    ** не закрывается до выхода). Нет библиотеки или функции — GpuException с подсказкой о пакетах.
    **
    ** Open libvulkan.so.1 and fill the pointers (once per process, thread-safe; the library
    ** is not closed until exit). No library or function — a GpuException with a hint about the packages.
    */
    void loadVulkan();
}

#endif
