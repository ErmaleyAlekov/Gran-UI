#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi::vk
{
    // Отдельная allocation на изображение автоматически соблюдает bufferImageGranularity.
    // A separate allocation per image automatically respects bufferImageGranularity.
    Image VulkanBackend::makeImage(const TextureDesc& desc, bool target)
    {
        Image image;
        image.width = desc.width;
        image.height = desc.height;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        if (desc.format == TextureFormat::R8) { format = VK_FORMAT_R8_UNORM; image.bytes = 1; }
        auto info = structure<VkImageCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent = {desc.width, desc.height, 1};
        info.mipLevels = 1; info.arrayLayers = 1; info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (target) info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        GRAN_CHECK_VK(vkCreateImage(_device.get(), &info, nullptr,
            image.handle.put(std::bind(vkDestroyImage, _device.get(), std::placeholders::_1, nullptr))));
        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(_device.get(), image.handle.get(), &requirements);
        auto allocation = structure<VkMemoryAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        GRAN_CHECK_VK(vkAllocateMemory(_device.get(), &allocation, nullptr,
            image.memory.put(std::bind(vkFreeMemory, _device.get(), std::placeholders::_1, nullptr))));
        GRAN_CHECK_VK(vkBindImageMemory(_device.get(), image.handle.get(), image.memory.get(), 0));
        imageView(image, format, target);
        nameImage(image, desc.debugName);
        return image;
    }

    // Framebuffer удалится до view, view — до image и памяти.
    // The framebuffer is destroyed before the view, the view before the image and memory.
    void VulkanBackend::imageView(Image& image, VkFormat format, bool target)
    {
        auto info = structure<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        info.image = image.handle.get(); info.viewType = VK_IMAGE_VIEW_TYPE_2D; info.format = format;
        info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        GRAN_CHECK_VK(vkCreateImageView(_device.get(), &info, nullptr,
            image.view.put(std::bind(vkDestroyImageView, _device.get(), std::placeholders::_1, nullptr))));
        if (!target) return;
        VkImageView view = image.view.get();
        auto framebuffer = structure<VkFramebufferCreateInfo>(VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
        framebuffer.renderPass = _pass.get(); framebuffer.attachmentCount = 1; framebuffer.pAttachments = &view;
        framebuffer.width = image.width; framebuffer.height = image.height; framebuffer.layers = 1;
        GRAN_CHECK_VK(vkCreateFramebuffer(_device.get(), &framebuffer, nullptr,
            image.framebuffer.put(std::bind(vkDestroyFramebuffer, _device.get(), std::placeholders::_1, nullptr))));
    }

    // Консервативные stage/access совместимы с Vulkan 1.0 старых систем, без synchronization2.
    // Conservative stage/access masks support Vulkan 1.0 on older systems without synchronization2.
    void VulkanBackend::barrier(Image& image, VkImageLayout layout)
    {
        auto transition = structure<VkImageMemoryBarrier>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
        transition.oldLayout = image.layout; transition.newLayout = layout;
        transition.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transition.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transition.image = image.handle.get(); transition.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        if (image.layout != VK_IMAGE_LAYOUT_UNDEFINED)
            transition.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        transition.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        vkCmdPipelineBarrier(_frames[_current].commands, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &transition);
        image.layout = layout;
    }

    // Цель и уровни размытия пересоздаются только после waitFrames.
    // The target and blur levels are recreated only after waitFrames.
    void VulkanBackend::createTargets()
    {
        TextureDesc desc{_options.width, _options.height, TextureFormat::RGBA8, "target"};
        _target = makeImage(desc, true);
        for (unsigned level = 0; level < BLUR_LEVELS; ++level)
        {
            if (level > 0) { desc.width = std::max(1u, desc.width / 2); desc.height = std::max(1u, desc.height / 2); }
            _blur[level] = makeImage(desc, true);
        }
    }

    // Новая текстура получает определённое (нулевое) содержимое на GPU, без копии в памяти процессора.
    // A new texture gets defined (zero) contents on the GPU, without a copy in CPU memory.
    void VulkanBackend::clearTexture(Image& image)
    {
        VkClearColorValue zero{};
        VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        barrier(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkCmdClearColorImage(_frames[_current].commands, image.handle.get(), image.layout, &zero, 1, &range);
        image.cleared = true;
    }
}
