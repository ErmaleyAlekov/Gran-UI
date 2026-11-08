#include "gran/rhi/dx11/internal/dx11_backend_impl.hpp"
#include "gran/rhi/dx12/internal/dx12_internal.hpp"
#include "gran/rhi/adapter_choice.hpp"

#include <string>

namespace gran::rhi::dx11
{
    namespace
    {
        /*
        ** Аппаратный адаптер, имя которого содержит GRAN_ADAPTER; порядок — как у DXGI (основной дисплей первым).
        ** Не нашёлся — ошибка со списком просмотренных имён.
        **
        ** The hardware adapter whose name contains GRAN_ADAPTER; the order is DXGI's (the main display first).
        ** Not found — an error with the list of the names looked at.
        */
        ComRef<IDXGIAdapter1> pickRequested()
        {
            ComRef<IDXGIFactory1> factory;
            std::string seen;

            GRAN_DX11_CHECK(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(factory.put())));
            for (UINT index = 0;; ++index)
            {
                ComRef<IDXGIAdapter1> adapter;
                DXGI_ADAPTER_DESC1 desc{};
                if (factory->EnumAdapters1(index, adapter.put()) == DXGI_ERROR_NOT_FOUND)
                    adapterNotFound(seen);
                adapter->GetDesc1(&desc);
                std::string name = dx12::describeDxgiAdapter(adapter.get(), "Direct3D 11").name;
                seen += name + "; ";
                if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && adapterAllowed(name))
                    return adapter;
            }
        }

        // Фабрика DXGI того адаптера, на котором создано устройство (для цепочки обмена).
        // The DXGI factory of the adapter the device was created on (for the swapchain).
        ComRef<IDXGIFactory2> deviceFactory(ID3D11Device* device)
        {
            ComRef<IDXGIDevice> dxgi;
            ComRef<IDXGIAdapter> adapter;
            ComRef<IDXGIFactory2> factory;

            GRAN_DX11_CHECK(device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgi.put())));
            GRAN_DX11_CHECK(dxgi->GetAdapter(adapter.put()));
            GRAN_DX11_CHECK(adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(factory.put())));
            return factory;
        }
    }

    // FL11_0 гарантирует StructuredBuffer и Shader Model 5.0; адаптер задан — тип драйвера UNKNOWN.
    // FL11_0 guarantees StructuredBuffer and Shader Model 5.0; with a given adapter the driver type is UNKNOWN.
    HRESULT Dx11Backend::tryDevice(IDXGIAdapter* adapter, D3D_DRIVER_TYPE type, UINT flags)
    {
        D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};

        return D3D11CreateDevice(adapter, type, nullptr, flags, levels, 1,
            D3D11_SDK_VERSION, _device.put(), nullptr, _context.put());
    }

    /*
    ** Отладочный слой необязателен. WARP — явно либо при отказе оборудования по умолчанию; карта,
    ** выбранная GRAN_ADAPTER, на WARP не заменяется (тест должен идти именно на ней).
    **
    ** The debug layer is optional. WARP is used explicitly or after a failure of the default hardware;
    ** a card chosen by GRAN_ADAPTER is not replaced by WARP (the test must run on exactly that card).
    */
    void Dx11Backend::createDevice()
    {
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_DRIVER_TYPE type = D3D_DRIVER_TYPE_HARDWARE;
        ComRef<IDXGIAdapter1> chosen;

        if (_options.forceWarp)
            type = D3D_DRIVER_TYPE_WARP;
        else if (!requestedAdapter().empty())
        {
            chosen = pickRequested();
            type = D3D_DRIVER_TYPE_UNKNOWN;
        }
        if (_options.debugLayer)
            flags |= D3D11_CREATE_DEVICE_DEBUG;
        HRESULT result = tryDevice(chosen.get(), type, flags);
        if (result == DXGI_ERROR_SDK_COMPONENT_MISSING)
            result = tryDevice(chosen.get(), type, D3D11_CREATE_DEVICE_BGRA_SUPPORT);
        if (FAILED(result) && type == D3D_DRIVER_TYPE_HARDWARE)
            result = tryDevice(nullptr, D3D_DRIVER_TYPE_WARP, D3D11_CREATE_DEVICE_BGRA_SUPPORT);
        GRAN_DX11_CHECK(result);
        _factory = deviceFactory(_device.get());
        // Без отладочного слоя E_NOINTERFACE ожидаем и не считаем ошибкой.
        // E_NOINTERFACE is expected without the debug layer and is not an error.
        HRESULT infoResult = _device->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(_info.put()));
        if (infoResult != E_NOINTERFACE)
            GRAN_DX11_CHECK(infoResult);
    }
}
