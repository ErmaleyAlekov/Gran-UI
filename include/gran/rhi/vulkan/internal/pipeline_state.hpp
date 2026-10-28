#ifndef GRAN_RHI_VULKAN_PIPELINE_STATE_HPP
#define GRAN_RHI_VULKAN_PIPELINE_STATE_HPP
#include "gran/rhi/vulkan/internal/vulkan_api.hpp"

namespace gran::rhi::vk
{
    // Структуры хранятся вместе, чтобы указатели VkGraphicsPipelineCreateInfo оставались валидны.
    // Keep state structures together so VkGraphicsPipelineCreateInfo pointers remain valid.
    struct PipelineState
    {
        explicit PipelineState(bool blur);
        VkPipelineVertexInputStateCreateInfo vertex{};
        VkPipelineInputAssemblyStateCreateInfo assembly{};
        VkPipelineViewportStateCreateInfo viewport{};
        VkPipelineRasterizationStateCreateInfo raster{};
        VkPipelineMultisampleStateCreateInfo samples{};
        VkPipelineColorBlendAttachmentState attachment{};
        VkPipelineColorBlendStateCreateInfo blend{};
        VkPipelineDynamicStateCreateInfo dynamic{};
        VkDynamicState states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkGraphicsPipelineCreateInfo info{};
    };
}
#endif
