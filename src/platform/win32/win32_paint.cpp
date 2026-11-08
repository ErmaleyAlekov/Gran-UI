#include "gran/platform/win32/win32_window.hpp"

namespace gran::platform::win32
{
    namespace
    {
        // Канал sRGB 0…1 → байт 0…255 / An sRGB channel 0…1 → a byte 0…255
        BYTE channelByte(float value) noexcept
        {
            if (value <= 0.0f)
                return 0;
            if (value >= 1.0f)
                return 255;
            return static_cast<BYTE>(value * 255.0f + 0.5f);
        }
    }

    /*
    ** Зачем окну с цепочкой обмена GDI-фон (Т-15). Пока край тянут мышью, DWM показывает окно уже нового размера
    ** раньше, чем приходит кадр GPU этого размера. В выросшей полосе тогда видна поверхность перенаправления
    ** окна, а она не закрашена: белые и чёрные прямоугольники. Сплошной фон окна в этой поверхности превращает
    ** полосу в продолжение фона — как у браузеров. Кисть DC_BRUSH — системная, создавать и удалять её не нужно.
    **
    ** Why a window with a swap chain needs a GDI background (T-15). While the edge is dragged with the mouse, DWM
    ** shows the window of the new size earlier than the GPU frame of that size arrives. The grown strip then shows
    ** the window redirection surface, and it is not painted: white and black rectangles. The solid window
    ** background in that surface turns the strip into a continuation of the background — as browsers do. The
    ** DC_BRUSH brush is a system one, it needs no creation and deletion.
    */
    void paintBackground(const Win32Window& window, HDC dc) noexcept
    {
        RECT client{};
        const Color& color = window.desc.background;

        if (dc == nullptr || !GetClientRect(window.window, &client))
            return;
        SetDCBrushColor(dc, RGB(channelByte(color.r), channelByte(color.g), channelByte(color.b)));
        FillRect(dc, &client, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    }

    // Кадр рисует GPU; GDI только стирает фон, и область помечается действительной
    // The GPU draws the frame; GDI only erases the background, and the area is marked valid
    void handlePaint(Win32Window& window) noexcept
    {
        PAINTSTRUCT paint{};

        if (BeginPaint(window.window, &paint) != nullptr)
            EndPaint(window.window, &paint);
        window.push(Event{});
    }
}
