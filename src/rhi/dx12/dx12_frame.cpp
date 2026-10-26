#include "gran/rhi/dx12/internal/dx12_backend_impl.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <string>

namespace gran::rhi::dx12
{
    /*
    ** Проверка до записи на GPU (правило 4): сегменты лежат внутри массива экземпляров,
    ** есть отсечение 0, указатели не пусты при ненулевых количествах. Шейдер читает
    ** корневые SRV без границ, поэтому выход за массив здесь — порча памяти GPU.
    **
    ** The check before writing to the GPU (rule 4): the segments lie inside the instance array,
    ** clip 0 exists, the pointers are not null with non-zero counts. The shader reads
    ** the root SRVs without bounds, so going beyond the array here corrupts GPU memory.
    */
    void Dx12Backend::validate(const FrameData& frame) const
    {
        bool valid = frame.clipCount >= 1 && frame.clips != nullptr
            && (frame.instanceCount == 0 || frame.instances != nullptr)
            && (frame.segmentCount == 0 || frame.segments != nullptr) && frame.rampRows >= 1;

        for (std::uint32_t index = 0; valid && index < frame.segmentCount; ++index)
        {
            const FrameSegment& segment = frame.segments[index];
            valid = segment.first <= frame.instanceCount && segment.count <= frame.instanceCount - segment.first;
        }
        if (!valid)
            throw GpuException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::GPU_BUFFER_OVERFLOW,
                {"frame", std::to_string(frame.instanceCount), std::to_string(frame.segmentCount)}), 0);
    }

    // Кадр ждёт свой прошлый забор: только после этого его распределитель и кольцо свободны
    // The frame waits for its previous fence: only then its allocator and ring are free
    FrameContext& Dx12Backend::beginFrame(const FrameData& frame)
    {
        FrameContext& context = _frames[_frameIndex];
        std::vector<std::uint32_t> freed;

        _fence.wait(context.fenceValue);
        _releases.collect(_fence.completed(), freed);
        _textures.reclaim(_context.device.get(), freed);
        GRAN_CHECK_HR(context.allocator->Reset());
        GRAN_CHECK_HR(_list->Reset(context.allocator.get(), _pipelines.ui.get()));
        std::uint64_t needed = static_cast<std::uint64_t>(frame.instanceCount) * sizeof(render::GpuInstance)
            + static_cast<std::uint64_t>(frame.clipCount) * sizeof(render::GpuClip) + 1024 + _textures.pendingBytes();
        context.ring.prepare(_context.device.get(), needed, "gran.uploadRing");
        return context;
    }

    // Отправка, показ, сигнал; отладочные сообщения собираются после каждого кадра
    // Submitting, presenting, the signal; the debug messages are collected after every frame
    FrameStatus Dx12Backend::submit(FrameContext& context)
    {
        ID3D12CommandList* lists[] = {_list.get()};

        _context.queue->ExecuteCommandLists(1, lists);
        HRESULT presented = _surface->present(_options.vsync);
        context.fenceValue = _fence.signal(_context.queue.get());
        _lastSignaled = context.fenceValue;
        _frameIndex = (_frameIndex + 1) % FRAMES_IN_FLIGHT;
        if (_context.debug)
        {
            for (std::string& message : drainDebugMessages(_context.device.get()))
                _diagnostics.push_back(std::move(message));
        }
        if (isDeviceLoss(presented))
            return FrameStatus::DEVICE_LOST;
        if (presented == DXGI_STATUS_OCCLUDED)
            return FrameStatus::OCCLUDED;
        GRAN_CHECK_HR(presented);
        return FrameStatus::PRESENTED;
    }

    // Кадр целиком; потеря устройства в любом вызове превращается в статус, а не исключение
    // The whole frame; a device loss in any call turns into a status, not an exception
    FrameStatus Dx12Backend::render(const FrameData& frame)
    {
        validate(frame);
        try
        {
            FrameContext& context = beginFrame(frame);
            FrameAddresses addresses = uploadFrame(frame, context);
            recordDraws(frame, addresses);
            GRAN_CHECK_HR(_list->Close());
            return submit(context);
        }
        catch (const GpuException& error)
        {
            if (error.getCode() == ErrorCode::DEVICE_LOST)
                return FrameStatus::DEVICE_LOST;
            throw;
        }
    }
}
