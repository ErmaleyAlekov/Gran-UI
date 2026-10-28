#include "gran/platform/win32/win32_window.hpp"

#include <dwmapi.h>
#include <windowsx.h>

namespace gran::platform::win32
{
    namespace
    {
        // Hover приходит и от неклиентской кнопки: DWM использует его для меню Snap Layouts.
        // Hover also comes from the nonclient button: DWM uses it for the Snap Layouts menu.
        void moveCaptionPointer(Win32Window& self, LPARAM position) noexcept
        {
            POINT point{GET_X_LPARAM(position), GET_Y_LPARAM(position)};
            Event event;

            ScreenToClient(self.window, &point);
            event.kind = EventKind::MOUSE_MOVE;
            event.position = Vec2{point.x / self.currentScale, point.y / self.currentScale};
            self.push(event);
            if (!self.trackingNonClient)
            {
                TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT), TME_LEAVE | TME_NONCLIENT, self.window, 0};
                self.trackingNonClient = TrackMouseEvent(&track) != FALSE;
            }
        }
    }

    // Нажатие max идёт через обычный маршрутизатор; захват доставит отпускание даже за кнопкой.
    // A max press uses the regular router; capture delivers the release even outside the button.
    bool translateFrameMouse(Win32Window& self, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result) noexcept
    {
        result = 0;
        if (message == WM_NCMOUSEMOVE || message == WM_NCMOUSELEAVE)
        {
            DwmDefWindowProc(self.window, message, wParam, lParam, &result);
            if (message == WM_NCMOUSEMOVE)
                moveCaptionPointer(self, lParam);
            else
            {
                self.trackingNonClient = false;
                Event event;
                event.kind = EventKind::MOUSE_LEAVE;
                self.push(event);
            }
            return true;
        }
        if ((message != WM_NCLBUTTONDOWN && message != WM_NCLBUTTONDBLCLK) || wParam != HTMAXBUTTON)
            return false;
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(self.window, &point);
        SetForegroundWindow(self.window);
        translateMouse(self, WM_LBUTTONDOWN, 0, MAKELPARAM(point.x, point.y));
        return true;
    }
}
