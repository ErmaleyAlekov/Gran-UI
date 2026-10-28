#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <algorithm>

namespace gran::rhi::vk
{
    // Каждое начало прохода задаёт viewport и scissor заново, в том числе после размытия.
    // Each pass sets viewport and scissor again, including after blur.
    void VulkanBackend::beginTarget(Image& image, bool clear, const float* color)
    {
        barrier(image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        auto info = structure<VkRenderPassBeginInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
        info.renderPass = _pass.get(); info.framebuffer = image.framebuffer.get(); info.renderArea.extent = {image.width, image.height};
        VkCommandBuffer commands = _frames[_current].commands;
        vkCmdBeginRenderPass(commands, &info, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{0, 0, static_cast<float>(image.width), static_cast<float>(image.height), 0, 1};
        VkRect2D scissor{{0, 0}, {image.width, image.height}};
        vkCmdSetViewport(commands, 0, 1, &viewport); vkCmdSetScissor(commands, 0, 1, &scissor);
        if (!clear) return;
        VkClearAttachment attachment{};
        attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        std::copy(color, color + 4, attachment.clearValue.color.float32);
        VkClearRect rectangle{scissor, 0, 1};
        vkCmdClearAttachments(commands, 1, &attachment, 1, &rectangle);
    }

    // instanceBase передаётся в константах; firstInstance=0 одинаков для Direct3D и Vulkan.
    // Pass instanceBase in constants; firstInstance=0 is consistent across Direct3D and Vulkan.
    void VulkanBackend::drawBatch(const FrameData& data, unsigned first, unsigned count, unsigned image)
    {
        auto values = constants(_target.width, _target.height);
        values.instanceBase = first; values.clipCount = data.clipCount;
        values.textureCount = static_cast<unsigned>(_textures.size()); values.rampRows = data.rampRows;
        values.atlasSlot = data.atlasSlot; values.rampSlot = data.rampSlot;
        bind(data, image);
        VkCommandBuffer commands = _frames[_current].commands;
        vkCmdPushConstants(commands, _layout.get(), VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(values), &values);
        vkCmdDraw(commands, 4, count, 0, 0);
    }

    // Группировка только по смене картинки сохраняет порядок прозрачных слоёв.
    // Batching only on image changes preserves transparent layer order.
    void VulkanBackend::drawSegment(const FrameData& data, const FrameSegment& segment)
    {
        unsigned first = segment.first;
        unsigned end = first + segment.count;
        unsigned image = MAX_TEXTURES;
        for (unsigned index = first; index < end; ++index)
        {
            const auto& item = data.instances[index];
            if (item.kind != render::KIND_IMAGE || item.paint[0] == image) continue;
            if (image != MAX_TEXTURES) { drawBatch(data, first, index - first, image); first = index; }
            image = item.paint[0];
        }
        if (first < end) drawBatch(data, first, end - first, image);
    }

    // Буферы принадлежат текущему кадру; fence уже пройден в beginFrame.
    // Buffers belong to the current frame; its fence was already waited in beginFrame.
    void VulkanBackend::drawFrame(const FrameData& data)
    {
        Frame& frame = _frames[_current];
        ensureBuffer(frame.instances, static_cast<VkDeviceSize>(data.instanceCount) * sizeof(render::GpuInstance), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        ensureBuffer(frame.clips, static_cast<VkDeviceSize>(data.clipCount) * sizeof(render::GpuClip), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        writeBuffer(frame.instances, data.instances, static_cast<VkDeviceSize>(data.instanceCount) * sizeof(render::GpuInstance));
        writeBuffer(frame.clips, data.clips, static_cast<VkDeviceSize>(data.clipCount) * sizeof(render::GpuClip));
        uploadTextures();
        beginTarget(_target, true, data.clearColor);
        vkCmdBindPipeline(frame.commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines[0].get());
        for (unsigned index = 0; index < data.segmentCount; ++index)
        {
            const auto& segment = data.segments[index];
            if (segment.backdropBefore)
            {
                vkCmdEndRenderPass(frame.commands);
                backdrop(segment.backdropRadius);
                beginTarget(_target, false, nullptr);
                vkCmdBindPipeline(frame.commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines[0].get());
            }
            drawSegment(data, segment);
        }
        vkCmdEndRenderPass(frame.commands);
        copyToSwapchain();
    }
}
