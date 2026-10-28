#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi::vk
{
    // Имена помогают RenderDoc и слоям валидации; расширение не обязательно в runtime.
    // Names aid RenderDoc and validation layers; the extension is optional at runtime.
    void VulkanBackend::nameObject(VkObjectType type, std::uint64_t handle, const char* name)
    {
        if (_debug.get() == VK_NULL_HANDLE || handle == 0) return;
        auto set = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(_device.get(), "vkSetDebugUtilsObjectNameEXT"));
        if (set == nullptr) return;
        auto info = structure<VkDebugUtilsObjectNameInfoEXT>(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = type; info.objectHandle = handle; info.pObjectName = name;
        GRAN_CHECK_VK(set(_device.get(), &info));
    }

    // Связанные части изображения получают одно имя, включая allocation и framebuffer.
    // Related image objects share a name, including the allocation and framebuffer.
    void VulkanBackend::nameImage(Image& image, const char* name)
    {
        nameObject(VK_OBJECT_TYPE_DEVICE_MEMORY, handleValue(image.memory.get()), name);
        nameObject(VK_OBJECT_TYPE_IMAGE, handleValue(image.handle.get()), name);
        nameObject(VK_OBJECT_TYPE_IMAGE_VIEW, handleValue(image.view.get()), name);
        nameObject(VK_OBJECT_TYPE_FRAMEBUFFER, handleValue(image.framebuffer.get()), name);
    }

    // Постоянные объекты именуются после завершения создания устройства и поверхности.
    // Name persistent objects after device and surface creation is complete.
    void VulkanBackend::nameStatics()
    {
        nameObject(VK_OBJECT_TYPE_INSTANCE, handleValue(_instance.get()), "gran.instance");
        nameObject(VK_OBJECT_TYPE_PHYSICAL_DEVICE, handleValue(_physical), "gran.adapter");
        nameObject(VK_OBJECT_TYPE_DEVICE, handleValue(_device.get()), "gran.device");
        nameObject(VK_OBJECT_TYPE_QUEUE, handleValue(_queue), "gran.graphics");
        nameObject(VK_OBJECT_TYPE_RENDER_PASS, handleValue(_pass.get()), "gran.pass");
        nameObject(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, handleValue(_descriptorLayout.get()), "gran.descriptor.layout");
        nameObject(VK_OBJECT_TYPE_PIPELINE_LAYOUT, handleValue(_layout.get()), "gran.pipeline.layout");
        nameObject(VK_OBJECT_TYPE_SAMPLER, handleValue(_sampler.get()), "gran.linear.clamp");
        nameObject(VK_OBJECT_TYPE_SWAPCHAIN_KHR, handleValue(_swapchain.get()), "gran.swapchain");
        for (const auto& semaphore : _presented)
            nameObject(VK_OBJECT_TYPE_SEMAPHORE, handleValue(semaphore.get()), "gran.present.finished");
        for (auto image : _swapImages)
            nameObject(VK_OBJECT_TYPE_IMAGE, handleValue(image), "gran.swapchain.image");
    }
}
