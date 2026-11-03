#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"
#include "gran/rhi/dx12/internal/dx12_internal.hpp"

namespace gran::rhi::dx11
{
    // Адаптер устройства D3D11 — через его интерфейс DXGI / The D3D11 device adapter — through its DXGI interface
    AdapterInfo Dx11Backend::adapterInfo() const
    {
        dx12::ComRef<IDXGIDevice> device;
        dx12::ComRef<IDXGIAdapter> adapter;

        if (FAILED(_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(device.put())))
            || FAILED(device->GetAdapter(adapter.put())))
            return dx12::describeDxgiAdapter(nullptr, "Direct3D 11");
        return dx12::describeDxgiAdapter(adapter.get(), "Direct3D 11");
    }
}
