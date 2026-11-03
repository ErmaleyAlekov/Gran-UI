#include "gran/platform/x11/x11_window.hpp"
#include <X11/Xatom.h>
#include <algorithm>
#include <cstdint>

namespace gran::platform::x11
{
    /*
    ** Подсказки диспетчеру окон до показа (EWMH): владелец — WM_TRANSIENT_FOR (окно над ним, сворачивается
    ** вместе с ним); тип — DIALOG, UTILITY или NORMAL; состояния — MODAL у модального и SKIP_TASKBAR
    ** у подчинённого (как в Windows: у окна с владельцем нет кнопки в панели задач). Номер окна владельца
    ** общий для всех соединений с сервером, поэтому годится и из другого Display.
    **
    ** The hints to the window manager before showing (EWMH): the owner is WM_TRANSIENT_FOR (the window stays above it, minimizes
    ** together with it); the type is DIALOG, UTILITY or NORMAL; the states are MODAL for a modal window and SKIP_TASKBAR
    ** for an owned one (as on Windows: a window with an owner has no taskbar button). The owner window number
    ** is shared by all the server connections, so it fits from another Display too.
    */
    void X11Window::applyOwner(const WindowDesc& desc)
    {
        Atom type = atom("_NET_WM_WINDOW_TYPE_NORMAL");
        Atom states[2] = {0, 0};
        int count = 0;

        if (desc.kind == WindowKind::DIALOG || desc.modal)
            type = atom("_NET_WM_WINDOW_TYPE_DIALOG");
        else if (desc.kind == WindowKind::TOOL)
            type = atom("_NET_WM_WINDOW_TYPE_UTILITY");
        XChangeProperty(_native.display, _native.window, atom("_NET_WM_WINDOW_TYPE"), XA_ATOM, 32, PropModeReplace,
            reinterpret_cast<unsigned char*>(&type), 1);
        if (desc.owner == nullptr)
            return;
        XSetTransientForHint(_native.display, _native.window,
            static_cast<::Window>(reinterpret_cast<std::uintptr_t>(desc.owner->nativeHandle())));
        states[count++] = atom("_NET_WM_STATE_SKIP_TASKBAR");
        if (desc.modal)
            states[count++] = atom("_NET_WM_STATE_MODAL");
        XChangeProperty(_native.display, _native.window, atom("_NET_WM_STATE"), XA_ATOM, 32, PropModeReplace,
            reinterpret_cast<unsigned char*>(states), count);
    }

    /*
    ** Место до показа: по центру владельца (его угол переводится в координаты корня — у окна в рамке
    ** диспетчера свои x, y относительно рамки) или экрана; окно не выходит за экран. Позиция
    ** передаётся и подсказкой USPosition: иначе диспетчер окон ставит окно по своему правилу.
    **
    ** The place before showing: at the owner center (its corner is translated into root coordinates — a window in the
    ** window manager frame has its own x, y relative to the frame) or at the screen center; the window does not leave the screen.
    ** The position is passed by the USPosition hint too: otherwise the window manager places the window by its own rule.
    */
    void X11Window::place(const WindowDesc& desc)
    {
        Display* display = _native.display;
        int screen = DefaultScreen(display);
        int targetX = 0, targetY = 0;
        int targetWidth = DisplayWidth(display, screen), targetHeight = DisplayHeight(display, screen);
        XWindowAttributes owner{};
        ::Window child = 0;

        if (desc.position == WindowPosition::DEFAULT)
            return;
        if (desc.position == WindowPosition::CENTER_OWNER && desc.owner != nullptr)
        {
            ::Window handle = static_cast<::Window>(reinterpret_cast<std::uintptr_t>(desc.owner->nativeHandle()));
            if (XGetWindowAttributes(display, handle, &owner) != 0
                && XTranslateCoordinates(display, handle, DefaultRootWindow(display), 0, 0, &targetX, &targetY, &child))
            {
                targetWidth = owner.width;
                targetHeight = owner.height;
            }
        }
        int width = static_cast<int>(_pixels.width), height = static_cast<int>(_pixels.height);
        int x = std::max(0, std::min(targetX + (targetWidth - width) / 2, DisplayWidth(display, screen) - width));
        int y = std::max(0, std::min(targetY + (targetHeight - height) / 2, DisplayHeight(display, screen) - height));
        XSizeHints hints{};
        long supplied = 0;
        XGetWMNormalHints(display, _native.window, &hints, &supplied);
        hints.flags |= USPosition | PPosition; hints.x = x; hints.y = y;
        XSetWMNormalHints(display, _native.window, &hints);
        XMoveWindow(display, _native.window, x, y);
    }

    /*
    ** Активация через диспетчер окон (_NET_ACTIVE_WINDOW, источник 1 — программа): он поднимает окно,
    ** разворачивает свёрнутое и отдаёт фокус. XRaiseWindow — для диспетчеров без EWMH.
    **
    ** Activation through the window manager (_NET_ACTIVE_WINDOW, source 1 — a program): it raises the window,
    ** restores a minimized one and gives the focus. XRaiseWindow is for window managers without EWMH.
    */
    void X11Window::activate()
    {
        XEvent event{};

        event.xclient.type = ClientMessage; event.xclient.window = _native.window;
        event.xclient.message_type = atom("_NET_ACTIVE_WINDOW"); event.xclient.format = 32;
        event.xclient.data.l[0] = 1; event.xclient.data.l[1] = CurrentTime;
        XSendEvent(_native.display, DefaultRootWindow(_native.display), False,
            SubstructureRedirectMask | SubstructureNotifyMask, &event);
        XRaiseWindow(_native.display, _native.window);
        XFlush(_native.display);
    }
}
