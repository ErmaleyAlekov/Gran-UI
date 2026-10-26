#include "gran/rhi/dx12/internal/dx12_internal.hpp"

namespace gran::rhi::dx12
{
    // SRV двумерной текстуры; nullptr даёт нулевой дескриптор (выборка возвращает 0)
    // A 2D texture SRV; nullptr gives a null descriptor (sampling returns 0)
    void TextureTable::writeView(ID3D12Device* device, std::uint32_t slot, ID3D12Resource* resource, DXGI_FORMAT format)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};
        D3D12_CPU_DESCRIPTOR_HANDLE handle = _heap->GetCPUDescriptorHandleForHeapStart();

        view.Format = format;
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Texture2D.MipLevels = 1;
        handle.ptr += static_cast<SIZE_T>(slot) * _increment;
        device->CreateShaderResourceView(resource, &view, handle);
    }

    // Зарезервированный слот получает SRV ресурса бэкенда (формат цели) / A reserved slot gets an SRV of a backend resource (the target format)
    void TextureTable::bindReserved(ID3D12Device* device, std::uint32_t slot, ID3D12Resource* resource)
    {
        if (slot != render::TEXTURE_BACKDROP && (slot < BLUR_SLOT_FIRST || slot >= TEXTURE_SLOTS))
            throwBadTexture(slot, "bindReserved");
        writeView(device, slot, resource, TARGET_FORMAT);
    }

    // Куча для привязки / The heap for binding
    ID3D12DescriptorHeap* TextureTable::heap() const noexcept
    {
        return _heap.get();
    }

    // Граница занятых слотов: последний занятый + 1 / The bound of the used slots: the last used one + 1
    std::uint32_t TextureTable::usedSlots() const noexcept
    {
        std::uint32_t bound = 0;

        for (std::uint32_t slot = 0; slot < _used.size(); ++slot)
        {
            if (_used[slot])
                bound = slot + 1;
        }
        return bound;
    }
}
