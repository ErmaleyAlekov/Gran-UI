#include "gran/platform/x11/x11_window.hpp"

namespace gran::platform::x11
{
    // Обработчик вызывается C++-циклом, а не callback драйвера; исключения обрабатывает Application.
    // The C++ loop invokes this handler, not a driver callback; Application handles exceptions.
    void X11Window::dispatch(XEvent& event)
    {
        switch (event.type)
        {
            case Expose: push(EventKind::EXPOSE); break;
            case ClientMessage:
                if (event.xclient.message_type == atom("WM_PROTOCOLS")
                    && static_cast<Atom>(event.xclient.data.l[0]) == atom("WM_DELETE_WINDOW")) push(EventKind::CLOSE);
                break;
            case ConfigureNotify:
                _pixels = {static_cast<float>(event.xconfigure.width), static_cast<float>(event.xconfigure.height)};
                { Event resized; resized.kind = EventKind::RESIZE;
                    resized.width = event.xconfigure.width; resized.height = event.xconfigure.height; _events.push_back(resized); }
                break;
            case FocusIn: if (_native.input != nullptr) XSetICFocus(_native.input); push(EventKind::FOCUS_GAINED); break;
            case FocusOut: if (_native.input != nullptr) XUnsetICFocus(_native.input); push(EventKind::FOCUS_LOST); break;
            case LeaveNotify: push(EventKind::MOUSE_LEAVE); break;
            case MotionNotify: case ButtonPress: case ButtonRelease: mouse(event); break;
            case KeyPress: keyboard(event.xkey, true); break;
            case KeyRelease: keyboard(event.xkey, false); break;
            case SelectionRequest: case SelectionNotify: case SelectionClear: selection(event); break;
            default: break;
        }
    }
}
