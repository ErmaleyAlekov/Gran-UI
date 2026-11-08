#ifndef GRAN_RHI_DXGI_WINDOW_CHAIN_HPP
#define GRAN_RHI_DXGI_WINDOW_CHAIN_HPP

#include "gran/rhi/dx12/internal/com_ref.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dcomp.h>
#include <dxgi1_2.h>

namespace gran::rhi::dxgi
{
    using dx12::ComRef;

    /*
    ** Показ цепочки обмена окна через DirectComposition (Т-17). Цепочка, созданная прямо для HWND, на NVIDIA с
    ** аппаратными слоями (MPO) временами гасла: окно на мониторе становилось чёрным на 1–2 с, хотя запись
    ** экрана показывала нормальный кадр. Edge и Терминал Windows отдают свою цепочку в DirectComposition и этой
    ** ошибки не имеют — так делает и движок. Визуальный элемент ставится поверх поверхности окна (topmost), поэтому
    ** GDI-фон окна (Т-15) остаётся под ним и закрывает щель при растягивании.
    ** Поля объявлены в порядке создания и уничтожаются в обратном: визуальный элемент, цель, устройство DComp.
    **
    ** Presenting the window swap chain through DirectComposition (T-17). A chain created directly for an HWND
    ** sometimes went dark on NVIDIA with hardware planes (MPO): the window on the monitor turned black for 1–2 s
    ** while a screen recording showed a normal frame. Edge and Windows Terminal hand their chain to DirectComposition
    ** and do not have this bug — so does the engine. The visual is placed above the window surface (topmost), so the
    ** window GDI background (T-15) stays below it and covers the gap while stretching.
    ** The fields are declared in the creation order and destroyed in reverse: the visual, the target, the DComp device.
    */
    class CompositionTarget
    {
    public:
        // Цепочка — содержимое визуального элемента окна; S_OK — показ идёт через DirectComposition
        // The chain becomes the window visual content; S_OK — presenting goes through DirectComposition
        HRESULT attach(HWND window, IUnknown* chain);

        // Идёт ли показ через DirectComposition / Whether presenting goes through DirectComposition
        bool attached() const noexcept;

        // Отпустить визуальный элемент, цель и устройство (повторный вызов безопасен)
        // Release the visual, the target and the device (a repeated call is safe)
        void reset() noexcept;

    private:
        ComRef<IDCompositionDevice> _device;
        ComRef<IDCompositionTarget> _target;
        ComRef<IDCompositionVisual> _visual;
    };

    // Что нужно для цепочки окна (не больше 5 параметров у функции) / What a window chain needs (at most 5 function parameters)
    struct WindowChainRequest
    {
        IDXGIFactory2* factory = nullptr;
        // D3D12 — очередь команд, D3D11 — устройство / D3D12 — the command queue, D3D11 — the device
        IUnknown* device = nullptr;
        HWND window = nullptr;
        DXGI_SWAP_CHAIN_DESC1 desc{};
    };

    /*
    ** Цепочка окна: через DirectComposition, а если она недоступна или выключена (GRAN_PRESENT=hwnd —
    ** переключатель диагностики) — прямо для HWND, как раньше. Код ошибки — последнего способа.
    **
    ** The window chain: through DirectComposition, and if it is unavailable or switched off (GRAN_PRESENT=hwnd —
    ** a diagnostic switch) — directly for the HWND, as before. The error code is of the last way.
    */
    HRESULT createWindowChain(const WindowChainRequest& request, CompositionTarget& composition, IDXGISwapChain1** chain);
}

#endif
