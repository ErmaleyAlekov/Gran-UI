#include "gran/platform/x11/x11_window.hpp"
#include <X11/Xatom.h>
#include <cstdlib>

namespace gran::platform::x11
{
    // Реальное состояние читается у WM, включая изменения горячими клавишами рабочего стола.
    // Read actual WM state, including changes made by desktop shortcuts.
    bool X11Window::isMaximized() const
    {
        Atom type = 0; int format = 0; unsigned long count = 0; unsigned long remaining = 0;
        unsigned char* bytes = nullptr;
        int result = XGetWindowProperty(_native.display, _native.window, atom("_NET_WM_STATE"), 0, 64, False,
            XA_ATOM, &type, &format, &count, &remaining, &bytes);
        bool vertical = false; bool horizontal = false;
        if (result == Success && format == 32 && type == XA_ATOM)
        {
            auto states = reinterpret_cast<Atom*>(bytes);
            for (unsigned long index = 0; index < count; ++index)
            { vertical |= states[index] == atom("_NET_WM_STATE_MAXIMIZED_VERT"); horizontal |= states[index] == atom("_NET_WM_STATE_MAXIMIZED_HORZ"); }
        }
        if (bytes != nullptr) XFree(bytes);
        return vertical && horizontal;
    }

    // Движок отдаёт только роль; системный WM выполняет перемещение и resize.
    // The engine supplies only the role; the system WM performs movement and resizing.
    void X11Window::setFrameHitTest(std::function<WindowRole(Vec2)> callback) { _frameHit = std::move(callback); }
    void X11Window::setLiveResize(std::function<void()> callback) { _liveResize = std::move(callback); }

    // Порядок направлений соответствует EWMH _NET_WM_MOVERESIZE.
    // Direction values follow EWMH _NET_WM_MOVERESIZE.
    void X11Window::moveResize(const XButtonEvent& button, long direction)
    {
        XUngrabPointer(_native.display, button.time);
        XEvent event{};
        event.xclient.type = ClientMessage; event.xclient.window = _native.window;
        event.xclient.message_type = atom("_NET_WM_MOVERESIZE"); event.xclient.format = 32;
        event.xclient.data.l[0] = button.x_root; event.xclient.data.l[1] = button.y_root;
        event.xclient.data.l[2] = direction; event.xclient.data.l[3] = button.button; event.xclient.data.l[4] = 1;
        XSendEvent(_native.display, DefaultRootWindow(_native.display), False,
            SubstructureRedirectMask | SubstructureNotifyMask, &event);
        XFlush(_native.display);
    }

    // Края custom-окна и drag-роль принадлежат WM; интерактивные потомки остаются клиентскими.
    // The WM owns custom window edges and drag roles; interactive descendants remain client input.
    bool X11Window::frameMouse(const XButtonEvent& event)
    {
        if (!_custom || event.button != 1) return false;
        long direction = frameEdge(event.x, event.y);
        if (direction >= 0) { moveResize(event, direction); return true; }
        if (!_frameHit || _frameHit({event.x / _scale, event.y / _scale}) != WindowRole::DRAG) return false;
        if (_lastButton == 1 && event.time - _lastClick < 400 && std::abs(event.x - _lastX) < 5 && std::abs(event.y - _lastY) < 5)
        { if (isMaximized()) restore(); else maximize(); _lastClick = 0; }
        else { moveResize(event, 8); _lastClick = event.time; _lastButton = 1; _lastX = event.x; _lastY = event.y; }
        return true;
    }
}
