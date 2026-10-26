#include "gran/rhi/dx12/internal/swapchain_surface.hpp"

namespace gran::rhi::dx12
{
    // Текущий задний буфер / The current back buffer
    ID3D12Resource* SwapchainSurface::target()
    {
        return _buffers[_swapchain->GetCurrentBackBufferIndex()].get();
    }

    // RTV текущего буфера / The RTV of the current buffer
    D3D12_CPU_DESCRIPTOR_HANDLE SwapchainSurface::view()
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();

        handle.ptr += static_cast<SIZE_T>(_swapchain->GetCurrentBackBufferIndex()) * _rtvSize;
        return handle;
    }

    // Буфер окна отдыхает в PRESENT / A window buffer rests in PRESENT
    D3D12_RESOURCE_STATES SwapchainSurface::restState() const
    {
        return D3D12_RESOURCE_STATE_PRESENT;
    }

    // После отрисовки окну ничего не нужно / The window needs nothing after drawing
    void SwapchainSurface::afterDraw(ID3D12GraphicsCommandList* list)
    {
        (void)list;
    }

    // С vsync — интервал 1; без него — 0 и tearing, если монитор и система его поддерживают
    // With vsync — interval 1; without it — 0 and tearing if the monitor and the system support it
    HRESULT SwapchainSurface::present(bool vsync)
    {
        if (vsync)
            return _swapchain->Present(1, 0);
        if (_tearing)
            return _swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
        return _swapchain->Present(0, 0);
    }
}
