#include "gran/rhi/dxgi/window_chain.hpp"

#include <cstdlib>
#include <cstring>

namespace gran::rhi::dxgi
{
    namespace
    {
        /*
        ** Цепочка для композиции: растяжение (STRETCH) и альфа без учёта (IGNORE) — окно непрозрачно,
        ** как и раньше. При ошибке привязки цепочка отпускается: вызывающий перейдёт к HWND.
        **
        ** A chain for composition: stretching (STRETCH) and ignored alpha (IGNORE) — the window is opaque,
        ** as before. On a binding error the chain is released: the caller falls back to the HWND.
        */
        HRESULT createComposed(const WindowChainRequest& request, CompositionTarget& composition, IDXGISwapChain1** chain)
        {
            DXGI_SWAP_CHAIN_DESC1 desc = request.desc;
            ComRef<IDXGISwapChain1> created;

            desc.Scaling = DXGI_SCALING_STRETCH;
            desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            HRESULT result = request.factory->CreateSwapChainForComposition(request.device, &desc, nullptr, created.put());
            if (FAILED(result))
                return result;
            result = composition.attach(request.window, created.get());
            if (FAILED(result))
            {
                composition.reset();
                return result;
            }
            *chain = created.get();
            (*chain)->AddRef();
            return S_OK;
        }
    }

    // Устройство DComp без устройства отрисовки: свои поверхности DComp движку не нужны
    // A DComp device without a rendering device: the engine needs no DComp surfaces of its own
    HRESULT CompositionTarget::attach(HWND window, IUnknown* chain)
    {
        HRESULT result = DCompositionCreateDevice(nullptr, __uuidof(IDCompositionDevice),
            reinterpret_cast<void**>(_device.put()));
        if (SUCCEEDED(result))
            result = _device->CreateTargetForHwnd(window, TRUE, _target.put());
        if (SUCCEEDED(result))
            result = _device->CreateVisual(_visual.put());
        if (SUCCEEDED(result))
            result = _visual->SetContent(chain);
        if (SUCCEEDED(result))
            result = _target->SetRoot(_visual.get());
        if (SUCCEEDED(result))
            result = _device->Commit();
        return result;
    }

    bool CompositionTarget::attached() const noexcept
    {
        return _visual.get() != nullptr;
    }

    // Обратный порядок создания / The reverse creation order
    void CompositionTarget::reset() noexcept
    {
        _visual.reset();
        _target.reset();
        _device.reset();
    }

    // GRAN_PRESENT=hwnd — старый показ прямо в окно (для сравнения на стенде и как обход)
    // GRAN_PRESENT=hwnd — the old presenting directly to the window (for comparing on the stand and as a workaround)
    HRESULT createWindowChain(const WindowChainRequest& request, CompositionTarget& composition, IDXGISwapChain1** chain)
    {
        const char* mode = std::getenv("GRAN_PRESENT");
        bool wanted = mode == nullptr || std::strcmp(mode, "hwnd") != 0;

        if (wanted && SUCCEEDED(createComposed(request, composition, chain)))
            return S_OK;
        return request.factory->CreateSwapChainForHwnd(request.device, request.window, &request.desc, nullptr, nullptr,
            chain);
    }
}
