#include "gran/rhi/dx12/internal/dx12_backend_impl.hpp"

#include <algorithm>
#include <string>

namespace gran::rhi::dx12
{
    namespace
    {
        // Уровень цепочки: копия кадра (0) и уменьшения вдвое; создаётся в состоянии чтения шейдером
        // A chain level: the frame copy (0) and the halvings; created in the shader read state
        void createLevel(ID3D12Device* device, BackdropChain& chain, std::uint32_t level)
        {
            D3D12_HEAP_PROPERTIES heap{};
            D3D12_RESOURCE_DESC desc{};
            D3D12_CLEAR_VALUE clear{};
            std::string name = "gran.backdrop" + std::to_string(level);

            heap.Type = D3D12_HEAP_TYPE_DEFAULT;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Width = std::max(1u, chain.width >> level);
            desc.Height = std::max(1u, chain.height >> level);
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = TARGET_FORMAT;
            desc.SampleDesc.Count = 1;
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            clear.Format = TARGET_FORMAT;
            GRAN_CHECK_HR(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clear, __uuidof(ID3D12Resource),
                reinterpret_cast<void**>(chain.levels[level].put())));
            setDebugName(chain.levels[level].get(), name.c_str());
            chain.states[level] = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }
    }

    // RTV уровня в куче цепочки / The level RTV in the chain heap
    D3D12_CPU_DESCRIPTOR_HANDLE backdropView(const BackdropChain& chain, std::uint32_t level) noexcept
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = chain.rtvHeap->GetCPUDescriptorHandleForHeapStart();

        handle.ptr += static_cast<SIZE_T>(level) * chain.rtvSize;
        return handle;
    }

    /*
    ** Цепочка создаётся при первой подложке и после смены размера цели (resize уже дождался
    ** простоя GPU и сбросил прежнюю цепочку). Результат размытия (уровень 1) видит шейдер
    ** через слот TEXTURE_BACKDROP, остальные уровни — через зарезервированные слоты.
    **
    ** The chain is created at the first backdrop and after a target size change (resize has already waited
    ** for the GPU to idle and dropped the former chain). The blur result (level 1) is seen by the shader
    ** through the TEXTURE_BACKDROP slot, the other levels — through the reserved slots.
    */
    void Dx12Backend::ensureBackdrop()
    {
        D3D12_RESOURCE_DESC target = _surface->target()->GetDesc();
        ID3D12Device* device = _context.device.get();
        D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_RTV, BLUR_LEVELS, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0};

        if (_backdrop.levels[0] && _backdrop.width == target.Width && _backdrop.height == target.Height)
            return;
        _backdrop = BackdropChain{};
        _backdrop.width = static_cast<std::uint32_t>(target.Width);
        _backdrop.height = target.Height;
        GRAN_CHECK_HR(device->CreateDescriptorHeap(&heap, __uuidof(ID3D12DescriptorHeap),
            reinterpret_cast<void**>(_backdrop.rtvHeap.put())));
        _backdrop.rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for (std::uint32_t level = 0; level < BLUR_LEVELS; ++level)
        {
            createLevel(device, _backdrop, level);
            device->CreateRenderTargetView(_backdrop.levels[level].get(), nullptr, backdropView(_backdrop, level));
            _textures.bindReserved(device, BLUR_SLOT_FIRST + level, _backdrop.levels[level].get());
        }
        _textures.bindReserved(device, render::TEXTURE_BACKDROP, _backdrop.levels[1].get());
    }
}
