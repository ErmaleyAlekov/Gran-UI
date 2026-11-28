#include "gran/rhi/dx12/internal/dx12_backend_impl.hpp"

namespace gran::rhi::dx12
{
    // Адаптер, на котором создано устройство (железо или WARP) / The adapter the device is created on (hardware or WARP)
    AdapterInfo Dx12Backend::adapterInfo() const
    {
        return describeDxgiAdapter(_context.adapter.get(), "Direct3D 12");
    }

    // Причина из самого устройства: потерянное устройство отвечает на GetDeviceRemovedReason
    // The reason from the device itself: a lost device still answers GetDeviceRemovedReason
    std::string Dx12Backend::lostReason() const
    {
        if (!_context.device)
            return std::string();
        return describeDeviceLoss(_context.device->GetDeviceRemovedReason());
    }

    std::uint64_t TextureTable::textureBytes() const
    {
        std::uint64_t bytes = 0;
        for (const auto& entry : _entries)
        {
            if (!entry.resource) continue;
            std::uint64_t pixels = std::uint64_t(entry.desc.width) * entry.desc.height;
            if (entry.desc.format == TextureFormat::RGBA8) pixels *= 4;
            bytes += pixels;
        }
        return bytes;
    }

    VideoMemoryInfo Dx12Backend::videoMemoryInfo() const
    {
        auto info = dxgiMemory(_context.adapter.get());
        info.textureBytes = _textures.textureBytes();
        for (const auto& frame : _frames) info.bufferBytes += frame.ring.capacity();
        return info;
    }
}
