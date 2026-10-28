#include "gran/rhi/vulkan/internal/pipeline_state.hpp"

namespace gran::rhi::vk
{
    // Premultiplied alpha, sRGB-числа в UNORM-цели; Kawase перезаписывает целевой пиксель.
    // Premultiplied alpha, sRGB values in a UNORM target; Kawase overwrites the target pixel.
    PipelineState::PipelineState(bool blur)
    {
        vertex.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        if (blur) assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport.viewportCount = 1; viewport.scissorCount = 1;
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.polygonMode = VK_POLYGON_MODE_FILL; raster.lineWidth = 1; raster.cullMode = VK_CULL_MODE_NONE;
        samples.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        attachment.blendEnable = !blur; attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; attachment.dstAlphaBlendFactor = attachment.dstColorBlendFactor;
        attachment.colorWriteMask = 15;
        blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend.attachmentCount = 1; blend.pAttachments = &attachment;
        dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic.dynamicStateCount = 2; dynamic.pDynamicStates = states;
        info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.pVertexInputState = &vertex; info.pInputAssemblyState = &assembly; info.pViewportState = &viewport;
        info.pRasterizationState = &raster; info.pMultisampleState = &samples;
        info.pColorBlendState = &blend; info.pDynamicState = &dynamic;
    }
}
