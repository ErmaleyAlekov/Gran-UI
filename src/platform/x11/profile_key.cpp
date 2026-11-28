#include "gran/platform/host.hpp"
#include <X11/Xlib.h>

namespace gran::platform
{
    // Проверяем Shift при запуске, не создавая окна приложения.
    // Check Shift at startup without creating an application window.
    bool bypassProfileKey() noexcept
    {
        Display* display = XOpenDisplay(nullptr);
        if (!display)
            return false;
        Window root{}, child{};
        int rx{}, ry{}, x{}, y{};
        unsigned modifiers{};
        const auto found = XQueryPointer(display, DefaultRootWindow(display), &root, &child, &rx, &ry, &x, &y, &modifiers);
        XCloseDisplay(display);
        return found && (modifiers & ShiftMask) != 0;
    }
}
