#include "gran/rhi/dx12/internal/dx12_internal.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::rhi::dx12
{
    // Событие ОС закрывается вместе с забором / The OS event is closed together with the fence
    FenceSync::~FenceSync()
    {
        if (_event != nullptr)
            CloseHandle(_event);
    }

    // Забор начинается с 0: первый signal даст 1 / The fence starts at 0: the first signal gives 1
    void FenceSync::init(ID3D12Device* device)
    {
        GRAN_CHECK_HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence),
            reinterpret_cast<void**>(_fence.put())));
        setDebugName(_fence.get(), "gran.fence");
        _event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (_event == nullptr)
            throw GpuException(ErrorCode::GPU, i18n::format(i18n::Message::GPU_CALL_FAILED,
                {"CreateEventW", "GetLastError", __FILE__, "0"}), static_cast<long long>(GetLastError()));
    }

    // Очередь выставит значение, когда выполнит всё отправленное до сигнала
    // The queue sets the value when it has executed everything submitted before the signal
    std::uint64_t FenceSync::signal(ID3D12CommandQueue* queue)
    {
        ++_value;
        GRAN_CHECK_HR(queue->Signal(_fence.get(), _value));
        return _value;
    }

    // Уже пройдено — без ожидания; иначе событие и блокировка потока / Already passed — no waiting; otherwise an event and blocking
    void FenceSync::wait(std::uint64_t value)
    {
        if (value == 0 || _fence->GetCompletedValue() >= value)
            return;
        GRAN_CHECK_HR(_fence->SetEventOnCompletion(value, _event));
        WaitForSingleObject(_event, INFINITE);
    }

    // Пройденное значение / The passed value
    std::uint64_t FenceSync::completed() const
    {
        return _fence->GetCompletedValue();
    }
}
