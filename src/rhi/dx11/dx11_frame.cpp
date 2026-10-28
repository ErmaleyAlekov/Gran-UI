#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"

#include <cmath>

namespace gran::rhi::dx11
{
    // Проверяем указатели, диапазоны, слоты и индексы до первого обращения к GPU.
    // Check pointers, ranges, slots and indices before the first GPU access.
    void Dx11Backend::validate(const FrameData& frame) const
    {
        if (!frame.clips || frame.clipCount == 0 || frame.rampRows == 0
            || frame.instanceCount > (128u * 1024u * 1024u) / sizeof(render::GpuInstance)
            || frame.clipCount > (128u * 1024u * 1024u) / sizeof(render::GpuClip)
            || (frame.instanceCount != 0 && !frame.instances) || (frame.segmentCount != 0 && !frame.segments))
            invalid("frame");
        if (frame.atlasSlot >= _textures.size() || frame.rampSlot >= _textures.size()
            || !_textures[frame.atlasSlot].resource || !_textures[frame.rampSlot].resource
            || frame.rampRows > _textures[frame.rampSlot].height)
            invalid("atlas/ramp");
        for (UINT index = 0; index < frame.segmentCount; ++index)
        {
            const FrameSegment& segment = frame.segments[index];
            if (segment.first > frame.instanceCount || segment.count > frame.instanceCount - segment.first
                || !std::isfinite(segment.backdropRadius) || segment.backdropRadius < 0)
                invalid("segment");
        }
        for (UINT index = 0; index < frame.instanceCount; ++index)
        {
            const render::GpuInstance& item = frame.instances[index];
            if (item.clip >= frame.clipCount || item.kind > render::KIND_BACKDROP)
                invalid("instance");
            if (item.kind == render::KIND_IMAGE && (item.paint[0] >= _textures.size() || !_textures[item.paint[0]].resource))
                invalid("image");
        }
    }

    // Потеря устройства превращается в общий статус для Application::recreateGraphics.
    // Device loss becomes the shared status consumed by Application::recreateGraphics.
    FrameStatus Dx11Backend::render(const FrameData& frame)
    {
        validate(frame);
        try
        {
            drawFrame(frame);
            HRESULT result = _device->GetDeviceRemovedReason();
            if (SUCCEEDED(result) && _swapchain)
                result = _swapchain->Present(static_cast<UINT>(_options.vsync), 0);
            if (deviceLost(result))
                return FrameStatus::DEVICE_LOST;
            if (result == DXGI_STATUS_OCCLUDED)
                return FrameStatus::OCCLUDED;
            GRAN_DX11_CHECK(result);
            if (!_swapchain)
                _context->Flush();
            return FrameStatus::PRESENTED;
        }
        catch (const GpuException& error)
        {
            if (error.getCode() == ErrorCode::DEVICE_LOST)
                return FrameStatus::DEVICE_LOST;
            throw;
        }
    }
}
