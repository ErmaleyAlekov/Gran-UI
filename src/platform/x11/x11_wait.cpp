#include "gran/platform/x11/x11_window.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <poll.h>
#include <unistd.h>

namespace gran::platform::x11
{
    // Только поток UI создаёт и уничтожает окна, поэтому список без замка.
    // Only the UI thread creates and destroys windows, so the list needs no lock.
    std::vector<X11Window*>& X11Window::registry()
    {
        static std::vector<X11Window*> windows;

        return windows;
    }

    // Событие уже в очереди окна или в буфере Xlib — спать нельзя.
    // An event is already in the window queue or in the Xlib buffer — sleeping is not allowed.
    bool X11Window::pending()
    {
        return !_events.empty() || XPending(_native.display) != 0;
    }

    /*
    ** poll ждёт соединения всех окон и pipe пробуждения этого окна (post из других потоков);
    ** в простое нет активного опроса. Иначе второе окно (openWindow) не реагировало бы, пока
    ** главное спит.
    **
    ** poll waits for the connections of all the windows and the wake pipe of this window (posts from other threads);
    ** idle never busy-polls. Otherwise a second window (openWindow) would not react while
    ** the main one sleeps.
    */
    void X11Window::waitEvents(double timeout)
    {
        std::vector<pollfd> descriptors{{_native.wakeRead, POLLIN, 0}};
        int milliseconds = -1;

        for (X11Window* window : registry())
        {
            if (window->pending())
                return;
            XFlush(window->_native.display);
            descriptors.push_back({ConnectionNumber(window->_native.display), POLLIN, 0});
        }
        if (timeout >= 0)
            milliseconds = static_cast<int>(std::min(2147483647.0, std::ceil(timeout * 1000)));
        int result = ::poll(descriptors.data(), descriptors.size(), milliseconds);
        if (result < 0 && errno == EINTR)
            return;
        if (descriptors[0].revents & POLLIN)
        {
            char bytes[128];
            while (::read(_native.wakeRead, bytes, sizeof(bytes)) > 0) {}
        }
    }
}
