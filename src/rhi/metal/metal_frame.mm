#include "gran/rhi/metal/internal/metal_backend_impl.hpp"

#include <cmath>
#include <cstring>

namespace gran::rhi::metal
{
    // Те же проверки, что в D3D11: индексы экземпляров, отсечений и текстур проверены на CPU до записи в буфер GPU
    // The same checks as in D3D11: the instance, clip and texture indices are checked on the CPU before the GPU buffer write
    void MetalBackend::validate(const FrameData& frame) const
    {
        if (frame.clips == nullptr || frame.clipCount == 0 || frame.rampRows == 0
            || frame.instanceCount > MAX_FRAME_BUFFER / sizeof(render::GpuInstance)
            || frame.clipCount > MAX_FRAME_BUFFER / sizeof(render::GpuClip)
            || (frame.instanceCount != 0 && frame.instances == nullptr) || (frame.segmentCount != 0 && frame.segments == nullptr))
            invalid("frame");
        if (frame.atlasSlot >= _textures.size() || frame.rampSlot >= _textures.size()
            || _textures[frame.atlasSlot].resource == nil || _textures[frame.rampSlot].resource == nil
            || frame.rampRows > _textures[frame.rampSlot].height)
            invalid("atlas/ramp");
        for (std::uint32_t index = 0; index < frame.segmentCount; ++index)
        {
            const FrameSegment& segment = frame.segments[index];
            if (segment.first > frame.instanceCount || segment.count > frame.instanceCount - segment.first
                || !std::isfinite(segment.backdropRadius) || segment.backdropRadius < 0)
                invalid("segment");
        }
        for (std::uint32_t index = 0; index < frame.instanceCount; ++index)
        {
            const render::GpuInstance& item = frame.instances[index];
            if (item.clip >= frame.clipCount || item.kind > render::KIND_BACKDROP)
                invalid("instance");
            if (item.kind == render::KIND_IMAGE && (item.paint[0] >= _textures.size() || _textures[item.paint[0]].resource == nil))
                invalid("image");
        }
    }

    // Кадр: ждём свободный кадр в полёте, пишем его буферы, записываем загрузки и проходы, отправляем.
    // Потерю устройства сообщает обработчик завершения прошлого кадра — тогда кадр не рисуется.
    // A frame: wait for a free frame in flight, write its buffers, record the uploads and the passes, submit.
    // A device loss is reported by the completion handler of a previous frame — then the frame is not drawn.
    FrameStatus MetalBackend::render(const FrameData& frame)
    {
        validate(frame);
        if (_shared->lost)
            return FrameStatus::DEVICE_LOST;
        dispatch_semaphore_wait(_shared->inFlight, DISPATCH_TIME_FOREVER);
        try
        {
            id<MTLCommandBuffer> buffer = [_queue commandBuffer];
            GRAN_METAL_CHECK(buffer != nil, nil, "commandBuffer");
            buffer.label = @"gran.metal.frame";
            uploadFrame(frame);
            encodeUploads(buffer);
            drawFrame(buffer, frame);
            submit(buffer);
        }
        catch (...)
        {
            // Кадр не отправлен — его место в полёте возвращается, иначе следующий кадр ждал бы вечно
            // The frame is not submitted — its in-flight place is returned, otherwise the next frame would wait forever
            dispatch_semaphore_signal(_shared->inFlight);
            throw;
        }
        _frameIndex = (_frameIndex + 1) % FRAMES_IN_FLIGHT;
        if (_shared->lost)
            return FrameStatus::DEVICE_LOST;
        return FrameStatus::PRESENTED;
    }

    // Экземпляры и отсечения — в буферы текущего кадра; семафор гарантирует, что GPU их уже не читает
    // The instances and the clips go into the current frame buffers; the semaphore guarantees the GPU no longer reads them
    void MetalBackend::uploadFrame(const FrameData& frame)
    {
        FrameBuffers& target = _frames[_frameIndex];
        std::size_t instanceBytes = static_cast<std::size_t>(frame.instanceCount) * sizeof(render::GpuInstance);
        std::size_t clipBytes = static_cast<std::size_t>(frame.clipCount) * sizeof(render::GpuClip);

        ensureCapacity(target.instances, instanceBytes, "gran.metal.instances");
        ensureCapacity(target.clips, clipBytes, "gran.metal.clips");
        if (instanceBytes != 0)
            std::memcpy(target.instances.contents, frame.instances, instanceBytes);
        std::memcpy(target.clips.contents, frame.clips, clipBytes);
    }

    // Буфер растёт до нужного размера (не меньше 4 КБ); старый держит командный буфер, пока его читает GPU
    // The buffer grows to the needed size (at least 4 KB); the old one is held by the command buffer while the GPU reads it
    void MetalBackend::ensureCapacity(id<MTLBuffer> __strong& buffer, std::size_t bytes, const char* name)
    {
        if (bytes > MAX_FRAME_BUFFER)
            invalid("buffer");
        if (buffer != nil && buffer.length >= bytes)
            return;
        std::size_t capacity = 4096;
        while (capacity < bytes)
            capacity *= 2;
        buffer = [_device newBufferWithLength:capacity options:MTLResourceStorageModeShared];
        GRAN_METAL_CHECK(buffer != nil, nil, "newBufferWithLength");
        buffer.label = [NSString stringWithUTF8String:name];
    }

    // Обработчик получает свою копию shared_ptr: общее состояние переживёт бэкенд, если GPU закончит позже
    // The handler gets its own shared_ptr copy: the shared state outlives the backend if the GPU finishes later
    void MetalBackend::submit(id<MTLCommandBuffer> buffer)
    {
        std::shared_ptr<SharedState> shared = _shared;

        [buffer addCompletedHandler:^(id<MTLCommandBuffer> done) {
            completed(*shared, done);
        }];
        [buffer commit];
        _last = buffer;
    }
}
