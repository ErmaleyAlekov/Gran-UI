#include "gran/platform/win32/win32_window.hpp"

#include <windowsx.h>

namespace gran::platform::win32
{
    namespace
    {
        // Края остаются системными зонами изменения размера, включая все четыре угла.
        // Edges remain system resize regions, including all four corners.
        LRESULT resizeEdge(const Win32Window& self, POINT point) noexcept
        {
            RECT frame{};

            if (!self.desc.resizable || IsZoomed(self.window) || !GetWindowRect(self.window, &frame))
                return HTCLIENT;
            UINT dpi = GetDpiForWindow(self.window);
            int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
            int horizontal = GetSystemMetricsForDpi(SM_CXFRAME, dpi) + padding;
            int vertical = GetSystemMetricsForDpi(SM_CYFRAME, dpi) + padding;
            int column = 1;
            int row = 1;

            if (point.x < frame.left + horizontal)
                column = 0;
            else if (point.x >= frame.right - horizontal)
                column = 2;
            if (point.y < frame.top + vertical)
                row = 0;
            else if (point.y >= frame.bottom - vertical)
                row = 2;
            static const LRESULT EDGES[3][3] = {{HTTOPLEFT, HTTOP, HTTOPRIGHT},
                {HTLEFT, HTCLIENT, HTRIGHT}, {HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT}};
            return EDGES[row][column];
        }
    }

    // Экранные координаты знаковые: монитор может находиться левее или выше основного.
    // Screen coordinates are signed: a monitor can be left of or above the primary monitor.
    LRESULT hitFrame(Win32Window& self, LPARAM position)
    {
        POINT point{GET_X_LPARAM(position), GET_Y_LPARAM(position)};
        LRESULT edge = resizeEdge(self, point);

        if (edge != HTCLIENT)
            return edge;
        if (!self.frameHitTest || !ScreenToClient(self.window, &point))
            return HTCLIENT;
        float scale = static_cast<float>(GetDpiForWindow(self.window)) / 96.0f;
        WindowRole role = self.frameHitTest(Vec2{point.x / scale, point.y / scale});
        if (role == WindowRole::DRAG)
            return HTCAPTION;
        if (role == WindowRole::MAXIMIZE && self.desc.resizable)
            return HTMAXBUTTON;
        return HTCLIENT;
    }
}
