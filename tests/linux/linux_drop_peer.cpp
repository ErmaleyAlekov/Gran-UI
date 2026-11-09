#include "linux_tests.hpp"
#include <X11/Xlib.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <poll.h>

namespace linux_tests
{
    namespace
    {
        // Список файлов броска: комментарий, кириллица и пробел в %XX, второй файл
        // The drop file list: a comment, Cyrillic and a space in %XX, a second file
        const char DROP_LIST[] = "# Gran XDND test\r\nfile:///tmp/%D0%BF%20a.txt\r\nfile://localhost/tmp/b.txt\r\n";

        // Источник XDND: своё соединение, окно-владелец XdndSelection и окно-приёмник
        // The XDND source: its own connection, the XdndSelection owner window and the target window
        struct Peer
        {
            Display* display = nullptr;
            ::Window source = 0;
            ::Window target = 0;
        };

        // Сообщение XDND приёмнику: data.l[0] — окно-источник, остальное — по виду сообщения
        // An XDND message to the target: data.l[0] is the source window, the rest depends on the message kind
        void sendToTarget(const Peer& peer, const char* type, long second, long third, long fifth)
        {
            XEvent event{};

            event.xclient.type = ClientMessage;
            event.xclient.display = peer.display;
            event.xclient.window = peer.target;
            event.xclient.message_type = XInternAtom(peer.display, type, False);
            event.xclient.format = 32;
            event.xclient.data.l[0] = static_cast<long>(peer.source);
            event.xclient.data.l[1] = second;
            event.xclient.data.l[2] = third;
            event.xclient.data.l[4] = fifth;
            XSendEvent(peer.display, peer.target, False, NoEventMask, &event);
            XFlush(peer.display);
        }

        // Ответ на запрос данных броска: text/uri-list в свойство запросившего окна, иначе отказ (None)
        // The answer to the drop data request: text/uri-list into the requester property, otherwise a refusal (None)
        void answerRequest(const Peer& peer, const XSelectionRequestEvent& request)
        {
            XEvent reply{};

            reply.xselection.type = SelectionNotify;
            reply.xselection.requestor = request.requestor;
            reply.xselection.selection = request.selection;
            reply.xselection.target = request.target;
            reply.xselection.time = request.time;
            reply.xselection.property = None;
            if (request.target == XInternAtom(peer.display, "text/uri-list", False))
            {
                XChangeProperty(peer.display, request.requestor, request.property, request.target, 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(DROP_LIST), static_cast<int>(std::strlen(DROP_LIST)));
                reply.xselection.property = request.property;
            }
            XSendEvent(peer.display, request.requestor, False, NoEventMask, &reply);
            XFlush(peer.display);
        }

        // Ждёт сообщение type (data.l[1]), отвечая на запросы данных; -1 — не пришло за 5 с (холодный старт WSLg бывает дольше 2 с)
        // Waits for the message type (data.l[1]) answering data requests; -1 — it did not come within 5 s (a cold WSLg start may take longer than 2 s)
        long waitReply(const Peer& peer, const char* type)
        {
            Atom wanted = XInternAtom(peer.display, type, False);
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

            while (std::chrono::steady_clock::now() < deadline)
            {
                if (XPending(peer.display) == 0)
                {
                    pollfd descriptor{ConnectionNumber(peer.display), POLLIN, 0};
                    ::poll(&descriptor, 1, 10);
                    continue;
                }
                XEvent event{};
                XNextEvent(peer.display, &event);
                if (event.type == SelectionRequest)
                    answerRequest(peer, event.xselectionrequest);
                else if (event.type == ClientMessage && event.xclient.message_type == wanted)
                    return event.xclient.data.l[1];
            }
            return -1;
        }
    }

    /*
    ** Процесс-источник XDND (как файловый менеджер): Enter → Position в точке (x, y) окна-приёмника →
    ** Status → Drop → Finished. Аргументы: окно, x, y (пиксели устройства), ожидание "1" (принять) или "0".
    ** Код 0 — приёмник ответил ожидаемо и в Status, и в Finished.
    **
    ** An XDND source process (as a file manager): Enter → Position at the point (x, y) of the target window →
    ** Status → Drop → Finished. Arguments: the window, x, y (device pixels), the expectation "1" (accept) or "0".
    ** Code 0 — the target answered as expected both in Status and in Finished.
    */
    int dropPeer(char** arguments)
    {
        Peer peer;
        int rootX = 0;
        int rootY = 0;
        ::Window child = 0;
        long expected = std::strtol(arguments[3], nullptr, 10);

        peer.display = XOpenDisplay(nullptr);
        require(peer.display != nullptr, "xdnd peer display");
        peer.target = static_cast<::Window>(std::strtoul(arguments[0], nullptr, 10));
        peer.source = XCreateSimpleWindow(peer.display, DefaultRootWindow(peer.display), 0, 0, 1, 1, 0, 0, 0);
        XSetSelectionOwner(peer.display, XInternAtom(peer.display, "XdndSelection", False), peer.source, CurrentTime);
        XTranslateCoordinates(peer.display, peer.target, DefaultRootWindow(peer.display), std::atoi(arguments[1]),
            std::atoi(arguments[2]), &rootX, &rootY, &child);
        sendToTarget(peer, "XdndEnter", 5L << 24, static_cast<long>(XInternAtom(peer.display, "text/uri-list", False)), 0);
        // Координаты по 16 бит без знака: окно WSLg бывает за краем корня, а сдвиг отрицательного — UB
        // Coordinates as unsigned 16 bits: a WSLg window may be beyond the root edge, and shifting a negative one is UB
        unsigned long packed = ((static_cast<unsigned long>(rootX) & 0xFFFFUL) << 16) | (static_cast<unsigned long>(rootY) & 0xFFFFUL);
        sendToTarget(peer, "XdndPosition", 0, static_cast<long>(packed),
            static_cast<long>(XInternAtom(peer.display, "XdndActionCopy", False)));
        long accepted = waitReply(peer, "XdndStatus");
        sendToTarget(peer, "XdndDrop", 0, CurrentTime, 0);
        long finished = waitReply(peer, "XdndFinished");
        XCloseDisplay(peer.display);
        if (accepted < 0 || finished < 0 || (accepted & 1) != expected || (finished & 1) != expected)
            return 3;
        return 0;
    }
}
