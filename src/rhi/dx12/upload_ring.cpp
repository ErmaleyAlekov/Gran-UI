#include "gran/rhi/dx12/internal/dx12_internal.hpp"

namespace gran::rhi::dx12
{
    // Буфер отображается до конца жизни кольца / The buffer stays mapped until the ring dies
    UploadRing::~UploadRing()
    {
        release();
    }

    // Отображение снимается до Release: ресурс не уничтожается отображённым / The mapping is removed before Release: the resource is not destroyed mapped
    void UploadRing::release() noexcept
    {
        if (_buffer && _mapped != nullptr)
            _buffer->Unmap(0, nullptr);
        _mapped = nullptr;
        _buffer.reset();
        _capacity = 0;
        _offset = 0;
    }

    /*
    ** Кольцо кадра используется только после ожидания его забора: GPU его не читает,
    ** поэтому старый буфер при росте можно освободить сразу, без отложенной очереди.
    **
    ** A frame ring is used only after waiting for its fence: the GPU does not read it,
    ** so the old buffer can be released at once when growing, without the deferred queue.
    */
    void UploadRing::prepare(ID3D12Device* device, std::uint64_t minimum, const char* name)
    {
        D3D12_HEAP_PROPERTIES heap{};
        D3D12_RESOURCE_DESC desc{};
        D3D12_RANGE noRead{0, 0};
        std::uint64_t capacity = 64 * 1024;

        _offset = 0;
        if (_buffer && _capacity >= minimum)
            return;
        while (capacity < minimum)
            capacity *= 2;
        release();
        heap.Type = D3D12_HEAP_TYPE_UPLOAD;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = capacity;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        GRAN_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(_buffer.put())));
        setDebugName(_buffer.get(), name);
        GRAN_CHECK_HR(_buffer->Map(0, &noRead, reinterpret_cast<void**>(&_mapped)));
        _capacity = capacity;
    }

    // Смещение выравнивается вверх (выравнивание — степень двойки); выход за ёмкость запрещён
    // The offset is aligned up (the alignment is a power of two); going beyond the capacity is forbidden
    bool UploadRing::allocate(std::uint64_t size, std::uint64_t alignment, UploadAllocation& allocation)
    {
        std::uint64_t start = (_offset + alignment - 1) & ~(alignment - 1);

        if (_mapped == nullptr || start + size > _capacity || start + size < start)
            return false;
        allocation.cpu = _mapped + start;
        allocation.gpu = _buffer->GetGPUVirtualAddress() + start;
        allocation.resource = _buffer.get();
        allocation.offset = start;
        _offset = start + size;
        return true;
    }
}
