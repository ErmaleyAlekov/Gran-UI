#include "gran/platform/x11/x11_window.hpp"

#include <X11/Xlib-xcb.h>
#include <xcb/xproto.h>
#include <cstdlib>

namespace gran::platform::x11
{
    // Проверенный запрос удерживает BadWindow в этом ответе, не меняя обработчик ошибок Xlib.
    // A checked request confines BadWindow to this reply without changing Xlib's error handler.
    void sendDropFinished(Display* display, const XEvent& reply)
    {
        const XClientMessageEvent& message = reply.xclient;
        xcb_client_message_event_t wire{};
        wire.response_type = XCB_CLIENT_MESSAGE;
        wire.format = 32;
        wire.window = message.window;
        wire.type = message.message_type;
        for (unsigned i = 0; i < 5; ++i)
            wire.data.data32[i] = message.data.l[i];

        XFlush(display);
        xcb_connection_t* connection = XGetXCBConnection(display);
        const auto cookie = xcb_send_event_checked(connection, 0, wire.window, XCB_EVENT_MASK_NO_EVENT,
            reinterpret_cast<const char*>(&wire));
        std::free(xcb_request_check(connection, cookie));
    }

    // Общий путь отмены при истечении срока ожидания / Cancel when the selection conversion times out
    void X11Window::expireDrop()
    {
        if (_dnd.converting && std::chrono::steady_clock::now() >= _dnd.deadline)
        {
            dndReply("XdndFinished", 0, nullptr);
            dndEnd(EventKind::DRAG_LEAVE);
        }
    }
}
