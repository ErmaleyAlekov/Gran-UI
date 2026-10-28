#include "gran/platform/x11/x11_window.hpp"
#include <X11/Xatom.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace gran::platform::x11
{
    // DPI задаётся Xft.dpi; GRAN_UI_SCALE позволяет явно выбрать масштаб в сеансе XWayland.
    // DPI comes from Xft.dpi; GRAN_UI_SCALE allows an explicit scale in an XWayland session.
    void X11Window::create(const WindowDesc& desc)
    {
        const char* resources = XResourceManagerString(_native.display);
        const char* dpi = nullptr;
        if (resources != nullptr) dpi = std::strstr(resources, "Xft.dpi:");
        if (dpi != nullptr) _scale = std::strtof(dpi + 8, nullptr) / 96.0f;
        const char* overrideScale = std::getenv("GRAN_UI_SCALE");
        if (overrideScale != nullptr) _scale = std::strtof(overrideScale, nullptr);
        if (!std::isfinite(_scale) || _scale < 0.5f || _scale > 4.0f) _scale = 1;
        _pixels = {std::max(1.0f, desc.width * _scale), std::max(1.0f, desc.height * _scale)};
        _custom = desc.titleBar == TitleBar::CUSTOM; _resizable = desc.resizable;
        _native.window = XCreateSimpleWindow(_native.display, DefaultRootWindow(_native.display), 0, 0,
            static_cast<unsigned>(_pixels.width), static_cast<unsigned>(_pixels.height), 0, 0, 0);
        XSelectInput(_native.display, _native.window, ExposureMask | StructureNotifyMask | FocusChangeMask
            | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | LeaveWindowMask | KeyPressMask | KeyReleaseMask | PropertyChangeMask);
        Atom closeAtom = atom("WM_DELETE_WINDOW");
        XSetWMProtocols(_native.display, _native.window, &closeAtom, 1);
        XChangeProperty(_native.display, _native.window, atom("_NET_WM_NAME"), atom("UTF8_STRING"), 8,
            PropModeReplace, reinterpret_cast<const unsigned char*>(desc.title.data()), static_cast<int>(desc.title.size()));
        XStoreName(_native.display, _native.window, desc.title.c_str());
    }

    // ICCCM задаёт min/fixed-size; Motif hints отключают системный заголовок custom-окна.
    // ICCCM sets minimum/fixed sizes; Motif hints disable the system title bar for custom windows.
    void X11Window::configure(const WindowDesc& desc)
    {
        XSizeHints hints{};
        hints.flags = PMinSize;
        hints.min_width = std::max(1, static_cast<int>(desc.minWidth * _scale));
        hints.min_height = std::max(1, static_cast<int>(desc.minHeight * _scale));
        if (!desc.resizable)
        {
            hints.flags |= PMaxSize;
            hints.min_width = static_cast<int>(_pixels.width); hints.max_width = hints.min_width;
            hints.min_height = static_cast<int>(_pixels.height); hints.max_height = hints.min_height;
        }
        XSetWMNormalHints(_native.display, _native.window, &hints);
        if (_custom)
        {
            unsigned long motif[] = {2, 0, 0, 0, 0};
            XChangeProperty(_native.display, _native.window, atom("_MOTIF_WM_HINTS"), atom("_MOTIF_WM_HINTS"), 32,
                PropModeReplace, reinterpret_cast<unsigned char*>(motif), 5);
        }
    }

    // Имена атомов интернируются сервером, внешнего реестра не требуется.
    // The server interns atom names; no external registry is required.
    Atom X11Window::atom(const char* name) const
    {
        return XInternAtom(_native.display, name, False);
    }
}
