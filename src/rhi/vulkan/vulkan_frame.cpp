#include "gran/rhi/vulkan/internal/vulkan_backend_impl.hpp"
#include <cmath>

namespace gran::rhi::vk
{
    // Проверки диапазонов выполняются до acquire, записи команд и обращения к драйверу.
    // Validate ranges before acquire, command recording and driver access.
    void VulkanBackend::validate(const FrameData& data) const
    {
        if ((data.instanceCount != 0 && data.instances == nullptr) || (data.clipCount != 0 && data.clips == nullptr)
            || (data.segmentCount != 0 && data.segments == nullptr) || data.rampRows == 0
            || static_cast<VkDeviceSize>(data.instanceCount) * sizeof(render::GpuInstance) > _properties.limits.maxStorageBufferRange
            || static_cast<VkDeviceSize>(data.clipCount) * sizeof(render::GpuClip) > _properties.limits.maxStorageBufferRange)
            invalid("frame");
        if (data.atlasSlot >= _textures.size() || data.rampSlot >= _textures.size()
            || _textures[data.atlasSlot].handle.get() == VK_NULL_HANDLE || _textures[data.rampSlot].handle.get() == VK_NULL_HANDLE
            || data.rampRows > _textures[data.rampSlot].height) invalid("atlas/ramp");
        for (unsigned index = 0; index < data.segmentCount; ++index)
        {
            const auto& segment = data.segments[index];
            if (segment.first > data.instanceCount || segment.count > data.instanceCount - segment.first
                || !std::isfinite(segment.backdropRadius) || segment.backdropRadius < 0) invalid("segment");
        }
        validateInstances(data);
    }

    // Проверяем все индексы, чтобы шейдер никогда не читал за пределами storage buffer.
    // Check every index so the shader never reads beyond a storage buffer.
    void VulkanBackend::validateInstances(const FrameData& data) const
    {
        for (unsigned index = 0; index < data.instanceCount; ++index)
        {
            const auto& item = data.instances[index];
            if ((item.clip != 0 && item.clip >= data.clipCount) || item.kind > render::KIND_BACKDROP) invalid("instance");
            if (item.kind == render::KIND_IMAGE && (item.paint[0] >= _textures.size()
                || _textures[item.paint[0]].handle.get() == VK_NULL_HANDLE)) invalid("image");
        }
    }

    // DEVICE_LOST передаётся приложению для пересоздания GPU-кэша из CPU-сцены.
    // DEVICE_LOST reaches the application so it can rebuild the GPU cache from its CPU scene.
    FrameStatus VulkanBackend::render(const FrameData& data)
    {
        validate(data);
        if (_options.width == 0 || _options.height == 0) return FrameStatus::OCCLUDED;
        try
        {
            if (_recreate) resize(_options.width, _options.height);
            beginFrame();
            // Окно успело сменить размер после resize: цепочка пересоздаётся и кадр повторяется сразу,
            // иначе он пропадал, а нового кадра без следующего события не было — окно оставалось чёрным
            // The window changed size after resize: the chain is recreated and the frame is retried at once,
            // otherwise it was lost and without a next event no new frame came — the window stayed black
            if (!acquire())
            {
                resize(_options.width, _options.height);
                if (!acquire()) return FrameStatus::OCCLUDED;
            }
            drawFrame(data);
            submit();
            FrameStatus status = present();
            _current = (_current + 1) % FRAME_COUNT;
            return status;
        }
        catch (const GpuException& error)
        {
            if (error.getNativeCode() == VK_ERROR_DEVICE_LOST) return FrameStatus::DEVICE_LOST;
            throw;
        }
    }
}
