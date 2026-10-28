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
        beginFrame();
        width = _target.width; height = _target.height;
        VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;
        Buffer buffer = makeBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        barrier(_target, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}; copy.imageExtent = {width, height, 1};
        VkCommandBuffer commands = _frames[_current].commands;
        vkCmdCopyImageToBuffer(commands, _target.handle.get(), _target.layout, buffer.handle.get(), 1, &copy);
        GRAN_CHECK_VK(vkEndCommandBuffer(commands));
        auto info = structure<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
        info.commandBufferCount = 1; info.pCommandBuffers = &commands;
        GRAN_CHECK_VK(vkQueueSubmit(_queue, 1, &info, VK_NULL_HANDLE));
        GRAN_CHECK_VK(vkQueueWaitIdle(_queue));
        void* mapped = nullptr;
        GRAN_CHECK_VK(vkMapMemory(_device.get(), buffer.memory.get(), 0, size, 0, &mapped));
        rgba.resize(static_cast<std::size_t>(size));
        std::memcpy(rgba.data(), mapped, rgba.size());
        vkUnmapMemory(_device.get(), buffer.memory.get());
    }
}
