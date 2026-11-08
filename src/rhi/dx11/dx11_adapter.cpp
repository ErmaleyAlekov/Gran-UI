#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"
#include "gran/rhi/dx12/internal/dx12_internal.hpp"

namespace gran::rhi::dx11
{
    // Адаптер устройства D3D11 — через его интерфейс DXGI / The D3D11 device adapter — through its DXGI interface
    AdapterInfo Dx11Backend::adapterInfo() const
    {
        dx12::ComRef<IDXGIDevice> device;
        dx12::ComRef<IDXGIAdapter> adapter;
        AdapterInfo info;

        if (FAILED(_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(device.put())))
            || FAILED(device->GetAdapter(adapter.put())))
            info = dx12::describeDxgiAdapter(nullptr, "Direct3D 11");
        else
            info = dx12::describeDxgiAdapter(adapter.get(), "Direct3D 11");
        if (_swapchain)
            info.present = "HWND";
        if (_composition.attached())
            info.present = "DirectComposition";
        return info;
    }

    // Причина из устройства D3D11 — тем же описанием, что у D3D12 / The reason from the D3D11 device — with the same description as D3D12
    std::string Dx11Backend::lostReason() const
    {
        if (!_device)
            return std::string();
        return dx12::describeDeviceLoss(_device->GetDeviceRemovedReason());
    }
}
