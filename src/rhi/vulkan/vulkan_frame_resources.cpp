#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"

namespace gran::rhi::vk
{
    // Пул и fence отдельные для каждого кадра; командный буфер принадлежит своему пулу.
    // Every frame has a separate pool and fence; its command buffer belongs to the pool.
    void VulkanBackend::createFrame(Frame& frame)
    {
        auto pool = structure<VkCommandPoolCreateInfo>(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
        pool.queueFamilyIndex = _family;
        GRAN_CHECK_VK(vkCreateCommandPool(_device.get(), &pool, nullptr,
            frame.pool.put(std::bind(vkDestroyCommandPool, _device.get(), std::placeholders::_1, nullptr))));
        auto commands = structure<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        commands.commandPool = frame.pool.get(); commands.commandBufferCount = 1; commands.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        GRAN_CHECK_VK(vkAllocateCommandBuffers(_device.get(), &commands, &frame.commands));
        auto fence = structure<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        GRAN_CHECK_VK(vkCreateFence(_device.get(), &fence, nullptr,
            frame.fence.put(std::bind(vkDestroyFence, _device.get(), std::placeholders::_1, nullptr))));
        auto semaphore = structure<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
        GRAN_CHECK_VK(vkCreateSemaphore(_device.get(), &semaphore, nullptr,
            frame.acquired.put(std::bind(vkDestroySemaphore, _device.get(), std::placeholders::_1, nullptr))));
        const VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32768},
            {VK_DESCRIPTOR_TYPE_SAMPLER, 16384}, {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 65536}};
        auto descriptors = structure<VkDescriptorPoolCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
        descriptors.maxSets = 16384; descriptors.poolSizeCount = 3; descriptors.pPoolSizes = sizes;
        GRAN_CHECK_VK(vkCreateDescriptorPool(_device.get(), &descriptors, nullptr,
            frame.descriptors.put(std::bind(vkDestroyDescriptorPool, _device.get(), std::placeholders::_1, nullptr))));
    }

    // Создание до первой отправки не требует ожидания GPU.
    // Creation before the first submission requires no GPU wait.
    void VulkanBackend::createFrames()
    {
        for (Frame& frame : _frames)
        {
            createFrame(frame);
            nameObject(VK_OBJECT_TYPE_COMMAND_POOL, handleValue(frame.pool.get()), "gran.frame.pool");
            nameObject(VK_OBJECT_TYPE_COMMAND_BUFFER, handleValue(frame.commands), "gran.frame.commands");
            nameObject(VK_OBJECT_TYPE_FENCE, handleValue(frame.fence.get()), "gran.frame.fence");
            nameObject(VK_OBJECT_TYPE_SEMAPHORE, handleValue(frame.acquired.get()), "gran.frame.acquire");
            nameObject(VK_OBJECT_TYPE_DESCRIPTOR_POOL, handleValue(frame.descriptors.get()), "gran.frame.descriptors");
        }
    }

    // Полное ожидание используется для resize, readback и разрушения, не в обычном кадре.
    // A full wait is used for resizing, readback and destruction, never in a normal frame.
    void VulkanBackend::waitFrames()
    {
        GRAN_CHECK_VK(vkDeviceWaitIdle(_device.get()));
        for (Frame& frame : _frames)
        {
            frame.submitted = false;
            frame.retired.clear();
        }
    }

    // Повторное использование только после fence: буферы, дескрипторы и отложенное удаление.
    // Reuse only after the fence: buffers, descriptors and deferred destruction.
    void VulkanBackend::beginFrame()
    {
        Frame& frame = _frames[_current];
        if (frame.submitted)
        {
            VkFence fence = frame.fence.get();
            GRAN_CHECK_VK(vkWaitForFences(_device.get(), 1, &fence, VK_TRUE, UINT64_MAX));
            frame.submitted = false;
        }
        frame.retired.clear();
        GRAN_CHECK_VK(vkResetCommandPool(_device.get(), frame.pool.get(), 0));
        GRAN_CHECK_VK(vkResetDescriptorPool(_device.get(), frame.descriptors.get(), 0));
        auto info = structure<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        GRAN_CHECK_VK(vkBeginCommandBuffer(frame.commands, &info));
    }

    // Fence сбрасывается лишь перед успешной попыткой отправки, а не до acquire.
    // Reset the fence only immediately before submission, never before acquire.
    void VulkanBackend::submit()
    {
        Frame& frame = _frames[_current];
        GRAN_CHECK_VK(vkEndCommandBuffer(frame.commands));
        VkFence fence = frame.fence.get();
        GRAN_CHECK_VK(vkResetFences(_device.get(), 1, &fence));
        auto info = structure<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
        info.commandBufferCount = 1; info.pCommandBuffers = &frame.commands;
        VkSemaphore wait = frame.acquired.get();
        VkSemaphore signal = VK_NULL_HANDLE;
        VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        if (_swapchain.get() != VK_NULL_HANDLE)
        {
            signal = _presented[_swapIndex].get();
            info.waitSemaphoreCount = 1; info.pWaitSemaphores = &wait; info.pWaitDstStageMask = &stage;
            info.signalSemaphoreCount = 1; info.pSignalSemaphores = &signal;
        }
        GRAN_CHECK_VK(vkQueueSubmit(_queue, 1, &info, fence));
        frame.submitted = true;
        _hasFrame = true;
    }
}
