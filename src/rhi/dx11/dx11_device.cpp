#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"

namespace gran::rhi::dx11
{
    // FL11_0 гарантирует StructuredBuffer и Shader Model 5.0.
    // FL11_0 guarantees StructuredBuffer and Shader Model 5.0.
    HRESULT Dx11Backend::tryDevice(D3D_DRIVER_TYPE type, UINT flags)
    {
        D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};

        return D3D11CreateDevice(nullptr, type, nullptr, flags, levels, 1,
            D3D11_SDK_VERSION, _device.put(), nullptr, _context.put());
    }

    // Отладочный слой необязателен, WARP выбирается явно либо при отказе оборудования.
    // The debug layer is optional; WARP is selected explicitly or after hardware failure.
    void Dx11Backend::createDevice()
    {
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_DRIVER_TYPE type = D3D_DRIVER_TYPE_HARDWARE;
        ComRef<IDXGIDevice> dxgi;
        ComRef<IDXGIAdapter> adapter;

        if (_options.forceWarp)
            type = D3D_DRIVER_TYPE_WARP;
        if (_options.debugLayer)
            flags |= D3D11_CREATE_DEVICE_DEBUG;
        HRESULT result = tryDevice(type, flags);
        if (result == DXGI_ERROR_SDK_COMPONENT_MISSING)
            result = tryDevice(type, D3D11_CREATE_DEVICE_BGRA_SUPPORT);
        if (FAILED(result) && type != D3D_DRIVER_TYPE_WARP)
            result = tryDevice(D3D_DRIVER_TYPE_WARP, D3D11_CREATE_DEVICE_BGRA_SUPPORT);
        GRAN_DX11_CHECK(result);
        GRAN_DX11_CHECK(_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgi.put())));
        GRAN_DX11_CHECK(dxgi->GetAdapter(adapter.put()));
        GRAN_DX11_CHECK(adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(_factory.put())));
        // Без отладочного слоя E_NOINTERFACE ожидаем и не считаем ошибкой.
        // E_NOINTERFACE is expected without the debug layer and is not an error.
        HRESULT infoResult = _device->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(_info.put()));
        if (infoResult != E_NOINTERFACE)
            GRAN_DX11_CHECK(infoResult);
    }
}
