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
            case ClientMessage: clientMessage(event.xclient); break;
            case ConfigureNotify: resized(event); break;
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

    /*
    ** Сообщения протоколов WM_PROTOCOLS: закрытие крестиком и запрос синхронизации смены размера (номер запроса —
    ** в data.l[2] (младшая часть) и data.l[3] (старшая)). Ответ на запрос — после кадра нового размера
    ** (x11_sync.cpp, framePresented).
    **
    ** The WM_PROTOCOLS messages: closing by the cross and the resize synchronization request (the request number is in
    ** data.l[2] (the low part) and data.l[3] (the high part)). The answer comes after a frame of the new size
    ** (x11_sync.cpp, framePresented).
    */
    void X11Window::clientMessage(const XClientMessageEvent& message)
    {
        if (message.message_type != atom("WM_PROTOCOLS"))
            return;
        Atom protocol = static_cast<Atom>(message.data.l[0]);
        if (protocol == atom("WM_DELETE_WINDOW"))
            push(EventKind::CLOSE);
        else if (protocol == atom("_NET_WM_SYNC_REQUEST") && _syncCounter != 0)
        {
            _syncLow = static_cast<unsigned long>(message.data.l[2]);
            _syncHigh = message.data.l[3];
            _syncPending = true;
            _syncConfigured = false;
        }
    }

    // Менеджер ждёт кадр нового размера: запрос пришёл, и после него — новый размер
    // The manager waits for a frame of the new size: a request came, and after it a new size
    bool X11Window::waitsForFrame() const
    {
        return _syncPending && _syncConfigured;
    }
}
