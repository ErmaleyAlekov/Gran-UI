#ifndef GRAN_RHI_DX12_INTERNAL_SWAPCHAIN_SURFACE_HPP
#define GRAN_RHI_DX12_INTERNAL_SWAPCHAIN_SURFACE_HPP

#include "gran/rhi/dx12/internal/dx12_internal.hpp"
#include "gran/rhi/dxgi/window_chain.hpp"

namespace gran::rhi::dx12
{
    /*
    ** Цепочка обмена окна: FLIP_DISCARD, три буфера, tearing при поддержке (VRR и режим без vsync); показ —
    ** через DirectComposition (window_chain.hpp, Т-17). Ссылки на буферы отпускаются перед ResizeBuffers —
    ** иначе DXGI откажет в изменении размера.
    **
    ** A window swap chain: FLIP_DISCARD, three buffers, tearing when supported (VRR and the no-vsync mode); presenting —
    ** through DirectComposition (window_chain.hpp, T-17). The buffer references are released before ResizeBuffers —
    ** otherwise DXGI refuses the resize.
    */
    class SwapchainSurface final : public Surface
    {
    public:
        // ---- swapchain_surface.cpp ----
        SwapchainSurface(DeviceContext& context, const BackendOptions& options);
        void resize(ID3D12Device* device, std::uint32_t width, std::uint32_t height) override;

        // ---- swapchain_present.cpp ----
        ID3D12Resource* target() override;
        D3D12_CPU_DESCRIPTOR_HANDLE view() override;
        D3D12_RESOURCE_STATES restState() const override;
        void afterDraw(ID3D12GraphicsCommandList* list) override;
        HRESULT present(bool vsync) override;

    private:
        // Буферы и их RTV (swapchain_surface.cpp) / The buffers and their RTVs (swapchain_surface.cpp)
        void acquireBuffers(ID3D12Device* device);

        ComRef<ID3D12DescriptorHeap> _rtvHeap;
        // Визуальный элемент DComp держит цепочку и объявлен раньше неё: уничтожается после, когда цепочка
        // уже отпущена своим полем
        // The DComp visual holds the chain and is declared before it: it is destroyed after, when the chain
        // has already been released by its own field
        dxgi::CompositionTarget _composition;
        ComRef<IDXGISwapChain3> _swapchain;
        ComRef<ID3D12Resource> _buffers[SWAP_BUFFERS];
        UINT _rtvSize = 0;
        UINT _flags = 0;
        bool _tearing = false;
    };
}

#endif
