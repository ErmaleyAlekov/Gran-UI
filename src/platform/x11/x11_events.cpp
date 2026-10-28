#include "gran/platform/x11/x11_window.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <poll.h>
#include <unistd.h>

namespace gran::platform::x11
{
    // Xlib читается только UI-потоком; wake использует независимый pipe.
    // Only the UI thread reads Xlib; wake uses an independent pipe.
    bool X11Window::pollEvent(Event& event)
    {
        while (_events.empty() && XPending(_native.display) != 0)
        {
            XEvent native{};
            XNextEvent(_native.display, &native);
            if (!XFilterEvent(&native, _native.window)) dispatch(native);
        }
        if (_events.empty()) return false;
        event = _events.front(); _events.pop_front();
        return true;
    }

    // Запись неблокирующая: заполненный pipe уже гарантирует пробуждение UI.
    // The write is nonblocking: a full pipe already guarantees the UI will wake.
    void X11Window::wake() noexcept
    {
        const char byte = 1;
        ssize_t result;
        do { result = ::write(_native.wakeWrite, &byte, 1); } while (result < 0 && errno == EINTR);
    }

    // Размер обновляется из ConfigureNotify, без синхронного roundtrip к X-серверу на каждый кадр.
    // ConfigureNotify updates size without a synchronous X-server roundtrip per frame.
    Size X11Window::pixelSize() const { return _pixels; }

    // Короткое событие без дополнительных данных.
    // A simple event without additional data.
    void X11Window::push(EventKind kind)
    {
        Event event;
        event.kind = kind;
        _events.push_back(event);
    }
}
