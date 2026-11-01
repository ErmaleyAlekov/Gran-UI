#include "gran/platform/x11/x11_window.hpp"
#include <cstdlib>

namespace gran::platform::x11
{
    // Колесо X11 — кнопки 4–7; остальные кнопки используют захват до отпускания.
    // X11 wheel events are buttons 4–7; ordinary buttons capture until release.
    void X11Window::mouse(XEvent& native)
    {
        Event event;
        event.kind = EventKind::MOUSE_MOVE;
        event.position = {native.xmotion.x / _scale, native.xmotion.y / _scale};
        event.modifiers = modifiers(native.xmotion.state);
        if (native.type == MotionNotify) { trackFrameCursor(native.xmotion); _events.push_back(event); return; }
        unsigned button = native.xbutton.button;
        if (button > 7) return;
        if (button >= 4 && button <= 7)
        {
            if (native.type != ButtonPress) return;
            event.kind = EventKind::MOUSE_WHEEL;
            if (button == 4) event.wheel.y = 1;
            if (button == 5) event.wheel.y = -1;
            if (button == 6) event.wheel.x = 1;
            if (button == 7) event.wheel.x = -1;
        }
        else
        {
            if (native.type == ButtonPress && frameMouse(native.xbutton)) return;
            this->button(native.xbutton, event);
        }
        _events.push_back(event);
    }

    // Захват удерживается до отпускания, а двойной щелчок требует близких координат.
    // Capture lasts until release, and a double-click requires nearby coordinates.
    void X11Window::button(const XButtonEvent& native, Event& event)
    {
        event.kind = EventKind::MOUSE_UP;
        if (native.type == ButtonPress) event.kind = EventKind::MOUSE_DOWN;
        if (native.button == 2) event.button = MouseButton::MIDDLE;
        if (native.button == 3) event.button = MouseButton::RIGHT;
        if (native.type == ButtonPress)
            XGrabPointer(_native.display, _native.window, False, ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, None, None, native.time);
        else XUngrabPointer(_native.display, native.time);
        if (native.type != ButtonPress) return;
        if (native.button == _lastButton && native.time - _lastClick < 400
            && std::abs(native.x - _lastX) < 5 && std::abs(native.y - _lastY) < 5) event.clicks = 2;
        _lastClick = native.time; _lastButton = native.button; _lastX = native.x; _lastY = native.y;
    }
}
