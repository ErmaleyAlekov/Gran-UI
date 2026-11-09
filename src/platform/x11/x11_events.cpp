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

    /*
    ** Пока край окна тянут мышью, оконный менеджер шлёт ConfigureNotify на каждое движение. Из скопившихся
    ** в очереди берётся только последний: каждый RESIZE стоит ожидания GPU и новой цепочки обмена, а размер
    ** промежуточных уже устарел (на стенде без слияния окно было чёрным в 238 замерах из 240).
    **
    ** While the window edge is dragged with the mouse, the window manager sends ConfigureNotify on every motion.
    ** Of those queued up only the last one is taken: every RESIZE costs a GPU wait and a new swap chain, and the
    ** size of the intermediate ones is already stale (on the stand without merging the window was black in 238 of
    ** 240 samples).
    */
    void X11Window::resized(XEvent& event)
    {
        XEvent next{};
        Event resize;

        while (XCheckTypedWindowEvent(_native.display, _native.window, ConfigureNotify, &next))
            event = next;
        // Новый размер после запроса синхронизации: следующий кадр можно отдавать менеджеру (x11_sync.cpp)
        // The new size after a sync request: the next frame may be handed to the manager (x11_sync.cpp)
        if (_syncPending)
            _syncConfigured = true;
        _pixels = {static_cast<float>(event.xconfigure.width), static_cast<float>(event.xconfigure.height)};
        resize.kind = EventKind::RESIZE;
        resize.width = event.xconfigure.width;
        resize.height = event.xconfigure.height;
        _events.push_back(resize);
    }

    // Короткое событие без дополнительных данных.
    // A simple event without additional data.
    void X11Window::push(EventKind kind)
    {
        Event event;
        event.kind = kind;
        _events.push_back(event);
    }
}
