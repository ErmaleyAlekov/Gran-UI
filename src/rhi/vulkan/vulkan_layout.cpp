#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi::vk
{
    // Фиксированные дескрипторы работают без descriptor indexing даже на старой Mesa.
    // Fixed descriptors work without descriptor indexing even on older Mesa.
    void VulkanBackend::createLayouts()
    {
        VkDescriptorSetLayoutBinding bindings[7]{};
        for (unsigned index = 0; index < 7; ++index)
        {
            bindings[index].binding = index; bindings[index].descriptorCount = 1;
            bindings[index].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
            bindings[index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            if (index < 2) bindings[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            if (index == 2) bindings[index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        }
        auto info = structure<VkDescriptorSetLayoutCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
        info.bindingCount = 7; info.pBindings = bindings;
        GRAN_CHECK_VK(vkCreateDescriptorSetLayout(_device.get(), &info, nullptr,
            _descriptorLayout.put(std::bind(vkDestroyDescriptorSetLayout, _device.get(), std::placeholders::_1, nullptr))));
        VkPushConstantRange range{VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(render::FrameConstants)};
        VkDescriptorSetLayout set = _descriptorLayout.get();
        auto layout = structure<VkPipelineLayoutCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
        layout.setLayoutCount = 1; layout.pSetLayouts = &set; layout.pushConstantRangeCount = 1; layout.pPushConstantRanges = &range;
        GRAN_CHECK_VK(vkCreatePipelineLayout(_device.get(), &layout, nullptr,
            _layout.put(std::bind(vkDestroyPipelineLayout, _device.get(), std::placeholders::_1, nullptr))));
        auto sampler = structure<VkSamplerCreateInfo>(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
        sampler.magFilter = VK_FILTER_LINEAR; sampler.minFilter = VK_FILTER_LINEAR;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; sampler.addressModeV = sampler.addressModeU; sampler.addressModeW = sampler.addressModeU;
        GRAN_CHECK_VK(vkCreateSampler(_device.get(), &sampler, nullptr,
            _sampler.put(std::bind(vkDestroySampler, _device.get(), std::placeholders::_1, nullptr))));
    }

    // LOAD сохраняет уже нарисованные сегменты после проходов размытия; очистка явная.
    // LOAD preserves previously drawn segments after blur passes; clearing is explicit.
    void VulkanBackend::createRenderPass()
    {
        VkAttachmentDescription attachment{};
        attachment.format = VK_FORMAT_R8G8B8A8_UNORM; attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; attachment.finalLayout = attachment.initialLayout;
        VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; subpass.colorAttachmentCount = 1; subpass.pColorAttachments = &reference;
        auto info = structure<VkRenderPassCreateInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
        info.attachmentCount = 1; info.pAttachments = &attachment; info.subpassCount = 1; info.pSubpasses = &subpass;
        GRAN_CHECK_VK(vkCreateRenderPass(_device.get(), &info, nullptr,
            _pass.put(std::bind(vkDestroyRenderPass, _device.get(), std::placeholders::_1, nullptr))));
    }
}
