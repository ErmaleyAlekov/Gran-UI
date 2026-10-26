#include "gran/rhi/dx12/internal/offscreen_surface.hpp"

namespace gran::rhi::dx12
{
    namespace
    {
        // Цель отрисовки в DEFAULT-куче, сразу в состоянии покоя COPY_SOURCE / A render target in the DEFAULT heap, at once in the COPY_SOURCE rest state
        void createTarget(ID3D12Device* device, std::uint32_t width, std::uint32_t height, ComRef<ID3D12Resource>& target)
        {
            D3D12_HEAP_PROPERTIES heap{};
            D3D12_RESOURCE_DESC desc{};
            D3D12_CLEAR_VALUE clear{};

            heap.Type = D3D12_HEAP_TYPE_DEFAULT;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Width = width;
            desc.Height = height;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = TARGET_FORMAT;
            desc.SampleDesc.Count = 1;
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            clear.Format = TARGET_FORMAT;
            GRAN_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                D3D12_RESOURCE_STATE_COPY_SOURCE, &clear, __uuidof(ID3D12Resource), reinterpret_cast<void**>(target.put())));
            setDebugName(target.get(), "gran.offscreenTarget");
        }

        // Буфер чтения: строки выровнены на 256 байт / The readback buffer: the rows are aligned to 256 bytes
        void createReadback(ID3D12Device* device, std::uint64_t size, ComRef<ID3D12Resource>& readback)
        {
            D3D12_HEAP_PROPERTIES heap{};
            D3D12_RESOURCE_DESC desc{};

            heap.Type = D3D12_HEAP_TYPE_READBACK;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Width = size;
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.SampleDesc.Count = 1;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            GRAN_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(readback.put())));
            setDebugName(readback.get(), "gran.readback");
        }
    }

    // Пересоздание цели, буфера чтения и RTV (GPU простаивает — гарантирует вызывающий)
    // Recreating the target, the readback buffer and the RTV (the GPU is idle — the caller guarantees it)
    void OffscreenSurface::resize(ID3D12Device* device, std::uint32_t width, std::uint32_t height)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heap{};

        if (!_rtvHeap)
        {
            heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            heap.NumDescriptors = 1;
            GRAN_CHECK_HR(device->CreateDescriptorHeap(&heap, __uuidof(ID3D12DescriptorHeap),
                reinterpret_cast<void**>(_rtvHeap.put())));
        }
        _width = width;
        _height = height;
        _pitch = (static_cast<std::uint64_t>(width) * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)
            & ~static_cast<std::uint64_t>(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
        createTarget(device, width, height, _target);
        createReadback(device, _pitch * height, _readback);
        device->CreateRenderTargetView(_target.get(), nullptr, _rtvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // Единственная цель / The single target
    ID3D12Resource* OffscreenSurface::target()
    {
        return _target.get();
    }

    // Её RTV / Its RTV
    D3D12_CPU_DESCRIPTOR_HANDLE OffscreenSurface::view()
    {
        return _rtvHeap->GetCPUDescriptorHandleForHeapStart();
    }
}
