#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <cstring>

namespace gran::rhi::vk
{
    // HOST_COHERENT исключает зависимость от размера nonCoherentAtomSize.
    // HOST_COHERENT removes any dependency on nonCoherentAtomSize.
    Buffer VulkanBackend::makeBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
    {
        Buffer buffer;
        buffer.size = std::max<VkDeviceSize>(size, 16);
        auto info = structure<VkBufferCreateInfo>(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        info.size = buffer.size;
        info.usage = usage;
        GRAN_CHECK_VK(vkCreateBuffer(_device.get(), &info, nullptr,
            buffer.handle.put(std::bind(vkDestroyBuffer, _device.get(), std::placeholders::_1, nullptr))));
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(_device.get(), buffer.handle.get(), &requirements);
        auto allocation = structure<VkMemoryAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memoryType(requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        GRAN_CHECK_VK(vkAllocateMemory(_device.get(), &allocation, nullptr,
            buffer.memory.put(std::bind(vkFreeMemory, _device.get(), std::placeholders::_1, nullptr))));
        GRAN_CHECK_VK(vkBindBufferMemory(_device.get(), buffer.handle.get(), buffer.memory.get(), 0));
        nameObject(VK_OBJECT_TYPE_BUFFER, handleValue(buffer.handle.get()), "gran.buffer");
        nameObject(VK_OBJECT_TYPE_DEVICE_MEMORY, handleValue(buffer.memory.get()), "gran.buffer.memory");
        return buffer;
    }

    // Проверяем границу до Map и копирования, пустые буферы допустимы для пустого кадра.
    // Check bounds before Map and copying; empty buffers are valid for an empty frame.
    void VulkanBackend::writeBuffer(Buffer& buffer, const void* bytes, VkDeviceSize size)
    {
        if (size > buffer.size || (size != 0 && bytes == nullptr)) invalid("buffer");
        if (size == 0) return;
        void* mapped = nullptr;
        GRAN_CHECK_VK(vkMapMemory(_device.get(), buffer.memory.get(), 0, size, 0, &mapped));
        std::memcpy(mapped, bytes, static_cast<std::size_t>(size));
        vkUnmapMemory(_device.get(), buffer.memory.get());
    }

    // Буфер кадра переиспользуется; рост — вдвое, чтобы не выделять память каждый кадр. Вызывается только
    // после ожидания fence этого кадра, поэтому прежний буфер GPU уже не читает.
    // A frame buffer is reused; it grows twofold so memory is not allocated every frame. Called only
    // after waiting for this frame fence, so the GPU no longer reads the former buffer.
    void VulkanBackend::ensureBuffer(Buffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage)
    {
        if (buffer.handle.get() != VK_NULL_HANDLE && size <= buffer.size)
            return;
        buffer = makeBuffer(std::max<VkDeviceSize>({size, buffer.size * 2, 65536}), usage);
    }
}
