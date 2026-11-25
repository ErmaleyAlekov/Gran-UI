#include "gran/platform/x11/x11_window.hpp"

#include <X11/Xatom.h>
#include <chrono>
#include <poll.h>

namespace gran::platform::x11
{
    namespace
    {
        struct TimestampRequest
        {
            ::Window window;
            Atom property;
            unsigned long serial;
        };

        // Более ранние ответы на внутренней метке можно удалить / Earlier replies on the private marker can be discarded
        Bool timestampEvent(Display*, XEvent* event, XPointer context)
        {
            const auto& request = *reinterpret_cast<const TimestampRequest*>(context);
            return event->type == PropertyNotify && !event->xproperty.send_event
                && event->xproperty.window == request.window && event->xproperty.atom == request.property
                && event->xproperty.state == PropertyNewValue && event->xproperty.serial <= request.serial;
        }
    }

    // Нулевая допись свойства возвращает время X-сервера / A zero-length append to a property returns X server time
    Time X11Window::clipboardTimestamp()
    {
        Display* display = _native.display;
        const Atom property = atom("GRAN_CLIPBOARD_TIMESTAMP");
        const unsigned char unused = 0;
        TimestampRequest request{_native.window, property, NextRequest(display)};
        XChangeProperty(display, _native.window, property, XA_STRING, 8, PropModeAppend, &unused, 0);
        XFlush(display);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        pollfd descriptor{ConnectionNumber(display), POLLIN, 0};
        XEvent event{};

        while (std::chrono::steady_clock::now() < deadline)
        {
            if (XCheckIfEvent(display, &event, timestampEvent, reinterpret_cast<XPointer>(&request)))
            {
                if (event.xproperty.serial == request.serial)
                    return event.xproperty.time;
                continue;
            }
            ::poll(&descriptor, 1, 10);
        }
        return CurrentTime;
    }
}
