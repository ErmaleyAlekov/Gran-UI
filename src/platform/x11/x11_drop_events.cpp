#include "gran/platform/x11/x11_window.hpp"
#include <cstdint>

namespace gran::platform::x11
{
    /*
    ** XdndPosition: точка указателя в координатах корня (data.l[2] = x << 16 | y) переводится в окно и в
    ** логические пиксели. Ответ XdndStatus — сразу и по зонам, без дерева UI (как приёмник OLE в Windows);
    ** бит 1 просит присылать положение и дальше, чтобы подсветка зоны шла за указателем. Приложению — DRAG_MOVE.
    **
    ** XdndPosition: the pointer point in root coordinates (data.l[2] = x << 16 | y) is translated into the window and
    ** into logical pixels. The XdndStatus answer is immediate and by the zones, without the UI tree (as the OLE receiver
    ** on Windows); bit 1 asks to keep sending the position so the zone highlight follows the pointer. The app gets DRAG_MOVE.
    */
    void X11Window::dndPosition(const XClientMessageEvent& message)
    {
        unsigned long packed = static_cast<unsigned long>(message.data.l[2]);
        // Половины — 16-битные числа со знаком, как INT16 в протоколе X / The halves are signed 16-bit numbers, as INT16 in the X protocol
        int rootX = static_cast<std::int16_t>((packed >> 16) & 0xFFFF);
        int rootY = static_cast<std::int16_t>(packed & 0xFFFF);
        int x = 0;
        int y = 0;
        ::Window child = 0;
        Event event;

        XTranslateCoordinates(_native.display, DefaultRootWindow(_native.display), _native.window, rootX, rootY,
            &x, &y, &child);
        _dnd.point = Vec2{static_cast<float>(x) / _scale, static_cast<float>(y) / _scale};
        _dnd.accepted = _dnd.uriList && dndInZone(_dnd.point);
        if (_dnd.accepted)
            dndReply("XdndStatus", 3, "XdndActionCopy");
        else
            dndReply("XdndStatus", 2, nullptr);
        event.kind = EventKind::DRAG_MOVE;
        event.position = _dnd.point;
        _events.push_back(event);
    }

    // Точка внутри одной из зон (правая и нижняя границы не входят) / The point is inside one of the zones (the right and bottom edges are excluded)
    bool X11Window::dndInZone(Vec2 point) const
    {
        for (const Rect& zone : _dropZones)
        {
            if (point.x >= zone.x && point.y >= zone.y && point.x < zone.x + zone.width && point.y < zone.y + zone.height)
                return true;
        }
        return false;
    }

    /*
    ** XdndDrop: принятый бросок запрашивает данные text/uri-list у владельца XdndSelection со временем из
    ** сообщения (data.l[2]); ответ придёт в SelectionNotify (dndData). Непринятый — сразу XdndFinished
    ** «не выполнено» и DRAG_LEAVE: подсветка зоны снимается, путей нет.
    **
    ** XdndDrop: an accepted drop requests the text/uri-list data from the XdndSelection owner with the time from
    ** the message (data.l[2]); the answer comes in SelectionNotify (dndData). A not accepted one gets XdndFinished
    ** "not done" and DRAG_LEAVE at once: the zone highlight is removed, there are no paths.
    */
    void X11Window::dndDrop(const XClientMessageEvent& message)
    {
        Time time = CurrentTime;

        logDndDrop(_dnd);
        if (!_dnd.accepted)
        {
            dndReply("XdndFinished", 0, nullptr);
            dndEnd(EventKind::DRAG_LEAVE);
            return;
        }
        if (_dnd.version >= 1)
            time = static_cast<Time>(message.data.l[2]);
        _dnd.converting = true;
        XConvertSelection(_native.display, atom("XdndSelection"), atom("text/uri-list"), atom("GRAN_XDND"),
            _native.window, time);
        XFlush(_native.display);
    }

    /*
    ** Ответ источнику: XdndStatus (флаги: бит 0 — принимаем, бит 1 — присылать положение) или XdndFinished
    ** (бит 0 — выполнено). action — имя действия (XdndActionCopy) или nullptr — None. Пустой прямоугольник
    ** в data.l[2..3]: источник не экономит сообщения, подсветка зависит от каждой точки.
    **
    ** An answer to the source: XdndStatus (flags: bit 0 — we accept, bit 1 — keep sending the position) or XdndFinished
    ** (bit 0 — done). action is the action name (XdndActionCopy) or nullptr — None. An empty rectangle
    ** in data.l[2..3]: the source does not save messages, the highlight depends on every point.
    */
    void X11Window::dndReply(const char* type, long accepted, const char* action)
    {
        XEvent reply{};
        XClientMessageEvent& message = reply.xclient;

        message.type = ClientMessage;
        message.display = _native.display;
        message.window = _dnd.source;
        message.message_type = atom(type);
        message.format = 32;
        message.data.l[0] = static_cast<long>(_native.window);
        message.data.l[1] = accepted;
        if (action != nullptr && message.message_type == atom("XdndStatus"))
            message.data.l[4] = static_cast<long>(atom(action));
        else if (action != nullptr)
            message.data.l[2] = static_cast<long>(atom(action));
        XSendEvent(_native.display, _dnd.source, False, NoEventMask, &reply);
        XFlush(_native.display);
    }

    // Конец сеанса: событие приложению (DRAG_LEAVE или DROP) и сброс состояния / The session end: an event for the app (DRAG_LEAVE or DROP) and a state reset
    void X11Window::dndEnd(EventKind kind)
    {
        Event event;

        event.kind = kind;
        event.position = _dnd.point;
        _events.push_back(event);
        _dnd = DndState{};
    }
}
