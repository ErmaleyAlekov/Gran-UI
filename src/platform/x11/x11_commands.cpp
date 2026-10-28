#include "gran/platform/x11/x11_window.hpp"
#include <X11/Xatom.h>

namespace gran::platform::x11
{
    // EWMH просит оконный менеджер изменить состояние, сохраняя правила рабочего стола.
    // EWMH asks the window manager to change state while respecting desktop policies.
    void X11Window::sendState(long state)
    {
        XEvent event{};
        event.xclient.type = ClientMessage; event.xclient.window = _native.window;
        event.xclient.message_type = atom("_NET_WM_STATE"); event.xclient.format = 32;
        event.xclient.data.l[0] = state; event.xclient.data.l[1] = atom("_NET_WM_STATE_MAXIMIZED_VERT");
        event.xclient.data.l[2] = atom("_NET_WM_STATE_MAXIMIZED_HORZ"); event.xclient.data.l[3] = 1;
        XSendEvent(_native.display, DefaultRootWindow(_native.display), False,
            SubstructureRedirectMask | SubstructureNotifyMask, &event);
        XFlush(_native.display);
    }
    void X11Window::maximize() { if (_resizable) sendState(1); }
    void X11Window::restore() { sendState(0); XMapRaised(_native.display, _native.window); }
    void X11Window::minimize() { XIconifyWindow(_native.display, _native.window, DefaultScreen(_native.display)); XFlush(_native.display); }

    // CLOSE обрабатывается приложением, а не немедленным XDestroyWindow под GPU.
    // The application handles CLOSE instead of immediately destroying the window beneath the GPU.
    void X11Window::close() { push(EventKind::CLOSE); }
}
