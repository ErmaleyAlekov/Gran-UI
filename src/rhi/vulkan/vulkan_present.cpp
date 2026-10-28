#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi::vk
{
    // Промежуточная RGBA-цель общая для offscreen и окна; blit преобразует RGBA в BGRA при необходимости.
    // An intermediate RGBA target is shared by offscreen and window rendering; blit converts RGBA to BGRA if needed.
    void VulkanBackend::copyToSwapchain()
    {
        if (_swapchain.get() == VK_NULL_HANDLE) return;
        barrier(_target, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        auto transition = structure<VkImageMemoryBarrier>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
        transition.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; transition.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        transition.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; transition.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transition.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        transition.image = _swapImages[_swapIndex]; transition.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkCommandBuffer commands = _frames[_current].commands;
        // srcStage совпадает со стадией ожидания семафора acquire (TRANSFER): только так смена раскладки
        // идёт после того, как presentation engine отдал изображение.
        // srcStage equals the acquire semaphore wait stage (TRANSFER): only then the layout change
        // runs after the presentation engine has released the image.
        vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &transition);
        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}; blit.dstSubresource = blit.srcSubresource;
        blit.srcOffsets[1] = {static_cast<int>(_target.width), static_cast<int>(_target.height), 1};
        blit.dstOffsets[1] = {static_cast<int>(_swapExtent.width), static_cast<int>(_swapExtent.height), 1};
        vkCmdBlitImage(commands, _target.handle.get(), _target.layout, transition.image, transition.newLayout, 1, &blit, VK_FILTER_NEAREST);
        transition.oldLayout = transition.newLayout; transition.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        transition.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; transition.dstAccessMask = 0;
        vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &transition);
    }
}
