#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi::vk
{
    // Каждый пакет получает неизменяемый набор до конца кадра: update не затрагивает команды в полёте.
    // Every batch gets an immutable set until the frame ends: updates cannot affect in-flight commands.
    VkDescriptorSet VulkanBackend::descriptors(const std::array<VkImageView, 4>& views)
    {
        Frame& frame = _frames[_current];
        VkDescriptorSet set = VK_NULL_HANDLE;
        VkDescriptorSetLayout layout = _descriptorLayout.get();
        auto allocation = structure<VkDescriptorSetAllocateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
        allocation.descriptorPool = frame.descriptors.get(); allocation.descriptorSetCount = 1; allocation.pSetLayouts = &layout;
        GRAN_CHECK_VK(vkAllocateDescriptorSets(_device.get(), &allocation, &set));
        nameObject(VK_OBJECT_TYPE_DESCRIPTOR_SET, handleValue(set), "gran.batch.descriptors");
        VkDescriptorBufferInfo buffers[] = {{frame.instances.handle.get(), 0, frame.instances.size}, {frame.clips.handle.get(), 0, frame.clips.size}};
        VkDescriptorImageInfo images[5]{};
        images[0].sampler = _sampler.get();
        for (unsigned index = 0; index < 4; ++index)
        { images[index + 1].imageView = views[index]; images[index + 1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; }
        VkWriteDescriptorSet writes[7]{};
        for (unsigned index = 0; index < 7; ++index)
        {
            auto& write = writes[index];
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; write.dstSet = set; write.dstBinding = index; write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            if (index < 2) { write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; write.pBufferInfo = &buffers[index]; }
            else { write.pImageInfo = &images[index - 2]; }
            if (index == 2) write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        }
        vkUpdateDescriptorSets(_device.get(), 7, writes, 0, nullptr);
        return set;
    }

    // Неиспользуемые привязки получают валидный атлас, а не null-дескриптор.
    // Unused bindings receive a valid atlas instead of a null descriptor.
    void VulkanBackend::bind(const FrameData& data, unsigned image)
    {
        VkImageView atlas = _textures[data.atlasSlot].view.get();
        std::array<VkImageView, 4> views{atlas, _textures[data.rampSlot].view.get(), atlas, atlas};
        if (image < _textures.size()) views[2] = _textures[image].view.get();
        if (_blur[1].layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) views[3] = _blur[1].view.get();
        VkDescriptorSet set = descriptors(views);
        vkCmdBindDescriptorSets(_frames[_current].commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _layout.get(), 0, 1, &set, 0, nullptr);
    }

    // Положительный viewport с инверсией Y в SPIR-V сохраняет координаты движка сверху вниз.
    // A positive viewport with SPIR-V Y inversion preserves the engine's top-down coordinates.
    render::FrameConstants VulkanBackend::constants(unsigned width, unsigned height) const
    {
        render::FrameConstants data{};
        data.viewport[0] = static_cast<float>(width); data.viewport[1] = static_cast<float>(height);
        data.inverseViewport[0] = 1.0f / data.viewport[0]; data.inverseViewport[1] = 1.0f / data.viewport[1];
        return data;
    }
}
