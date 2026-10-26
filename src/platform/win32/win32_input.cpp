#include "gran/platform/win32/win32_window.hpp"

#include <windowsx.h>

namespace gran::platform::win32
{
    namespace
    {
        // Кнопка и вид события по сообщению мыши; false — это не кнопка / The button and event kind by a mouse message; false — not a button
        bool mouseButton(UINT message, Event& event) noexcept
        {
            static const struct { UINT message; EventKind kind; MouseButton button; } BUTTONS[] = {
                {WM_LBUTTONDOWN, EventKind::MOUSE_DOWN, MouseButton::LEFT}, {WM_LBUTTONUP, EventKind::MOUSE_UP, MouseButton::LEFT},
                {WM_RBUTTONDOWN, EventKind::MOUSE_DOWN, MouseButton::RIGHT}, {WM_RBUTTONUP, EventKind::MOUSE_UP, MouseButton::RIGHT},
                {WM_MBUTTONDOWN, EventKind::MOUSE_DOWN, MouseButton::MIDDLE}, {WM_MBUTTONUP, EventKind::MOUSE_UP, MouseButton::MIDDLE},
                {WM_LBUTTONDBLCLK, EventKind::MOUSE_DOWN, MouseButton::LEFT}};

            for (const auto& entry : BUTTONS)
            {
                if (entry.message == message)
                {
                    event.kind = entry.kind;
                    event.button = entry.button;
                    if (message == WM_LBUTTONDBLCLK)
                        event.clicks = 2;
                    return true;
                }
            }
            return false;
        }

        // Колесо: координаты экранные — переводятся в клиентские; 120 единиц = один «щелчок»
        // The wheel: the coordinates are on screen — they are turned into client ones; 120 units = one "notch"
        void wheelEvent(const Win32Window& window, UINT message, WPARAM wParam, POINT& point, Event& event) noexcept
        {
            float notches = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;

            ScreenToClient(window.window, &point);
            event.kind = EventKind::MOUSE_WHEEL;
            if (message == WM_MOUSEWHEEL)
                event.wheel.y = notches;
            else
                event.wheel.x = notches;
        }
    }

    /*
    ** Мышь: координаты в логических пикселях; при нажатии окно захватывает мышь (перетаскивание
    ** ползунка за край окна), уход мыши отслеживается через TrackMouseEvent.
    **
    ** The mouse: the coordinates in logical pixels; on a press the window captures the mouse (dragging
    ** a slider beyond the window edge), the mouse leave is tracked through TrackMouseEvent.
    */
    bool translateMouse(Win32Window& window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Event event;
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

        if (message == WM_MOUSELEAVE)
        {
            window.trackingLeave = false;
            event.kind = EventKind::MOUSE_LEAVE;
        }
        else if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
            wheelEvent(window, message, wParam, point, event);
        else if (message == WM_MOUSEMOVE)
            event.kind = EventKind::MOUSE_MOVE;
        else if (!mouseButton(message, event))
            return false;
        if (event.kind == EventKind::MOUSE_MOVE && !window.trackingLeave)
        {
            TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT), TME_LEAVE, window.window, 0};
            window.trackingLeave = TrackMouseEvent(&track) != FALSE;
        }
        if (event.kind == EventKind::MOUSE_DOWN)
            SetCapture(window.window);
        else if (event.kind == EventKind::MOUSE_UP)
            ReleaseCapture();
        event.position = Vec2{point.x / window.currentScale, point.y / window.currentScale};
        event.modifiers = currentModifiers();
        window.push(event);
        return true;
    }
}
