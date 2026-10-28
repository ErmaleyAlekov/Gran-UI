#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <algorithm>
#include <cmath>

namespace gran::rhi::vk
{
    // Снимок подложки и та же цепочка Kawase, что у Direct3D.
    // Capture the backdrop and run the same Kawase chain as Direct3D.
    void VulkanBackend::backdrop(float radius)
    {
        unsigned passes = static_cast<unsigned>(std::lround(std::log2(std::max(2.0f, radius / 1.5f))));
        passes = std::clamp(passes, 1u, BLUR_LEVELS - 1);
        barrier(_target, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        barrier(_blur[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkImageCopy copy{};
        copy.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}; copy.dstSubresource = copy.srcSubresource;
        copy.extent = {_target.width, _target.height, 1};
        vkCmdCopyImage(_frames[_current].commands, _target.handle.get(), _target.layout,
            _blur[0].handle.get(), _blur[0].layout, 1, &copy);
        for (unsigned level = 1; level <= passes; ++level) blur(level - 1, level, 1);
        for (unsigned level = passes - 1; level > 0; --level) blur(level + 1, level, 2);
        barrier(_blur[1], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // Source и destination всегда различны; переходы описываются между проходами.
    // Source and destination are always different; transitions are specified between passes.
    void VulkanBackend::blur(unsigned source, unsigned destination, unsigned pipeline)
    {
        Image& input = _blur[source]; Image& output = _blur[destination];
        barrier(input, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        beginTarget(output, false, nullptr);
        VkCommandBuffer commands = _frames[_current].commands;
        vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines[pipeline].get());
        VkImageView view = input.view.get();
        VkDescriptorSet set = descriptors({view, view, view, view});
        vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _layout.get(), 0, 1, &set, 0, nullptr);
        auto values = constants(output.width, output.height);
        values.inverseViewport[0] = 1.0f / static_cast<float>(input.width);
        values.inverseViewport[1] = 1.0f / static_cast<float>(input.height);
        vkCmdPushConstants(commands, _layout.get(), VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(values), &values);
        vkCmdDraw(commands, 3, 1, 0, 0);
        vkCmdEndRenderPass(commands);
    }
}
