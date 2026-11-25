#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <cstring>

namespace gran::rhi::vk
{
    // Readback — явная синхронная операция вне обычного кадра; RGBA без выравнивания строк.
    // Readback is an explicit synchronous operation outside a normal frame; RGBA with tightly packed rows.
    void VulkanBackend::readPixels(std::vector<std::uint8_t>& rgba, unsigned& width, unsigned& height)
    {
        if (!_hasFrame) invalid("readPixels before render");
        waitFrames();
        const VkDeviceSize size = static_cast<VkDeviceSize>(_target.width) * _target.height * 4;
        if (size > rgba.max_size()) invalid("readPixels size");
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(size));
        auto buffer = std::make_unique<Buffer>(makeBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
        beginFrame();
        Frame& frame = _frames[_current];
        const auto oldLayout = _target.layout;
        barrier(_target, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}; copy.imageExtent = {_target.width, _target.height, 1};
        VkCommandBuffer commands = frame.commands;
        vkCmdCopyImageToBuffer(commands, _target.handle.get(), _target.layout, buffer->handle.get(), 1, &copy);
        auto hostRead = structure<VkMemoryBarrier>(VK_STRUCTURE_TYPE_MEMORY_BARRIER);
        hostRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        hostRead.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
            0, 1, &hostRead, 0, nullptr, 0, nullptr);
        _target.layout = oldLayout;
        GRAN_CHECK_VK(vkEndCommandBuffer(commands));
        auto info = structure<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
        info.commandBufferCount = 1; info.pCommandBuffers = &commands;
        VkFence fence = frame.fence.get();
        GRAN_CHECK_VK(vkResetFences(_device.get(), 1, &fence));
        const VkResult submitted = vkQueueSubmit(_queue, 1, &info, fence);
        if (submitted == VK_SUCCESS || submitted == VK_ERROR_DEVICE_LOST)
        {
            frame.submitted = true;
            frame.readback = std::move(buffer);
            _target.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        }
        GRAN_CHECK_VK(submitted);
        GRAN_CHECK_VK(vkWaitForFences(_device.get(), 1, &fence, VK_TRUE, UINT64_MAX));
        frame.submitted = false;
        buffer = std::move(frame.readback);
        void* mapped = nullptr;
        GRAN_CHECK_VK(vkMapMemory(_device.get(), buffer->memory.get(), 0, size, 0, &mapped));
        std::memcpy(pixels.data(), mapped, pixels.size());
        vkUnmapMemory(_device.get(), buffer->memory.get());
        rgba.swap(pixels);
        width = _target.width;
        height = _target.height;
    }
}
