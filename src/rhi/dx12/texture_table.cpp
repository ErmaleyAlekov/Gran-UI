#include "gran/rhi/dx12/internal/dx12_internal.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <string>
#include <utility>

namespace gran::rhi::dx12
{
    namespace
    {
        // Описание двумерной текстуры без мип-уровней / A 2D texture description without mip levels
        D3D12_RESOURCE_DESC textureResourceDesc(const TextureDesc& desc) noexcept
        {
            D3D12_RESOURCE_DESC resource{};

            resource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resource.Width = desc.width;
            resource.Height = desc.height;
            resource.DepthOrArraySize = 1;
            resource.MipLevels = 1;
            resource.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            if (desc.format == TextureFormat::R8)
                resource.Format = DXGI_FORMAT_R8_UNORM;
            resource.SampleDesc.Count = 1;
            return resource;
        }
    }

    // Куча на все слоты; слоты подложки и цепочки размытия заняты бэкендом с самого начала
    // A heap for all slots; the backdrop and blur chain slots are taken by the backend from the start
    void TextureTable::init(ID3D12Device* device)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc{};

        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = TEXTURE_SLOTS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        GRAN_CHECK_HR(device->CreateDescriptorHeap(&desc, __uuidof(ID3D12DescriptorHeap),
            reinterpret_cast<void**>(_heap.put())));
        setDebugName(_heap.get(), "gran.textureHeap");
        _increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        _entries.resize(TEXTURE_SLOTS);
        _used.assign(TEXTURE_SLOTS, false);
        for (std::uint32_t slot = 0; slot < TEXTURE_SLOTS; ++slot)
            writeView(device, slot, nullptr, DXGI_FORMAT_R8G8B8A8_UNORM);
        _used[render::TEXTURE_BACKDROP] = true;
        for (std::uint32_t slot = BLUR_SLOT_FIRST; slot < TEXTURE_SLOTS; ++slot)
            _used[slot] = true;
    }

    /*
    ** Ресурс в DEFAULT-куче (обнулён при создании) в состоянии COPY_DEST: в шейдер он попадёт
    ** только после recordUploads, переводящего его в PIXEL_SHADER_RESOURCE.
    **
    ** A resource in the DEFAULT heap (zeroed on creation) in the COPY_DEST state: it reaches the shader
    ** only after recordUploads moves it into PIXEL_SHADER_RESOURCE.
    */
    std::uint32_t TextureTable::create(ID3D12Device* device, const TextureDesc& desc)
    {
        D3D12_HEAP_PROPERTIES heap{};
        D3D12_RESOURCE_DESC resource = textureResourceDesc(desc);
        std::uint32_t slot = 0;

        while (slot < TEXTURE_SLOTS && _used[slot])
            ++slot;
        if (slot == TEXTURE_SLOTS)
            throw GpuException(ErrorCode::LIMIT_EXCEEDED, i18n::format(i18n::Message::GPU_DESCRIPTOR_EXHAUSTED,
                {std::to_string(TEXTURE_SLOTS)}), 0);
        if (desc.width == 0 || desc.height == 0 || desc.width > 16384 || desc.height > 16384)
            throwBadTexture(slot, "size");
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        TextureEntry& entry = _entries[slot];
        GRAN_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &resource,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(entry.resource.put())));
        setDebugName(entry.resource.get(), desc.debugName);
        entry.desc = desc;
        entry.state = D3D12_RESOURCE_STATE_COPY_DEST;
        writeView(device, slot, entry.resource.get(), resource.Format);
        _used[slot] = true;
        return slot;
    }

    // Ожидающие загрузки слота отменяются, ресурс ждёт забора кадра / The pending slot uploads are cancelled, the resource waits for the frame fence
    void TextureTable::release(std::uint32_t slot, DeferredReleases& releases, std::uint64_t fenceValue)
    {
        std::size_t kept = 0;

        if (slot >= TEXTURE_SLOTS || slot == render::TEXTURE_BACKDROP || !_entries[slot].resource)
            throwBadTexture(slot, "release");
        for (std::size_t index = 0; index < _pending.size(); ++index)
        {
            if (_pending[index].slot != slot)
                _pending[kept++] = std::move(_pending[index]);
        }
        _pending.resize(kept);
        releases.push(std::move(_entries[slot].resource), slot, fenceValue);
    }

    // Нулевой дескриптор и возврат слота / A null descriptor and the slot return
    void TextureTable::reclaim(ID3D12Device* device, const std::vector<std::uint32_t>& slots)
    {
        for (std::uint32_t slot : slots)
        {
            writeView(device, slot, nullptr, DXGI_FORMAT_R8G8B8A8_UNORM);
            _entries[slot] = TextureEntry{};
            _used[slot] = false;
        }
    }
}
