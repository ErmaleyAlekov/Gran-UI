#include "gran/platform/x11/x11_window.hpp"
#include <X11/Xatom.h>
#include <algorithm>
#include <utility>

namespace gran::platform::x11
{
    // Версия протокола XDND, которую понимает окно (5 — с ответом об успехе в XdndFinished)
    // The XDND protocol version the window understands (5 — with the success answer in XdndFinished)
    static const long DND_VERSION = 5;

    /*
    ** Зоны запоминаются всегда; свойство XdndAware ставится один раз, когда зоны впервые появились
    ** (как приёмник OLE в Windows): окно без зон источники обходят, и курсор «нельзя» рисует сам источник.
    **
    ** The zones are always kept; the XdndAware property is set once, when zones first appear
    ** (as the OLE receiver on Windows): sources skip a window without zones, and the source itself draws the "no" cursor.
    */
    void X11Window::setDropZones(const std::vector<Rect>& zones)
    {
        _dropZones = zones;
        if (zones.empty() || _dndAware)
            return;
        Atom version = static_cast<Atom>(DND_VERSION);
        XChangeProperty(_native.display, _native.window, atom("XdndAware"), XA_ATOM, 32, PropModeReplace,
            reinterpret_cast<unsigned char*>(&version), 1);
        XFlush(_native.display);
        _dndAware = true;
        logDndAware(_native.window, DND_VERSION);
    }

    // Самый ранний список путей; пусто — забирать нечего / The earliest path list; empty — nothing to take
    std::vector<std::string> X11Window::takeDroppedFiles()
    {
        std::vector<std::string> paths;

        if (_dropped.empty())
            return paths;
        paths = std::move(_dropped.front());
        _dropped.pop_front();
        return paths;
    }

    /*
    ** Сообщения источника XDND. Enter начинает сеанс; Position, Leave и Drop принимаются только
    ** от того же источника (data.l[0]) — чужое или запоздавшее сообщение не ломает текущий сеанс.
    **
    ** The XDND source messages. Enter starts a session; Position, Leave and Drop are accepted only
    ** from the same source (data.l[0]) — a foreign or late message does not break the current session.
    */
    void X11Window::dndMessage(const XClientMessageEvent& message)
    {
        ::Window source = static_cast<::Window>(message.data.l[0]);

        if (message.message_type == atom("XdndEnter"))
        {
            dndEnter(message);
            return;
        }
        if (source == 0 || source != _dnd.source)
            return;
        if (message.message_type == atom("XdndPosition"))
            dndPosition(message);
        else if (message.message_type == atom("XdndDrop"))
            dndDrop(message);
        // Leave после Drop, пока ждём данные, — не отмена: мост Mutter (GNOME на Wayland) шлёт его сразу, как только
        // кончилось перетаскивание на стороне Wayland. Сеанс кончит SelectionNotify (dndData), иначе данные терялись (Т-38)
        // A Leave after Drop while the data is awaited is not a cancel: the Mutter bridge (GNOME on Wayland) sends it right
        // after the drag on the Wayland side has ended. SelectionNotify (dndData) ends the session, otherwise the data was lost (T-38)
        else if (message.message_type == atom("XdndLeave") && !_dnd.converting)
            dndEnd(EventKind::DRAG_LEAVE);
    }

    /*
    ** Начало сеанса: источник, его версия (старший байт data.l[1]) и есть ли среди типов text/uri-list.
    ** Источник новее нашей версии говорит на нашей (так велит протокол), поэтому его версия не отсекается.
    **
    ** The session start: the source, its version (the high byte of data.l[1]) and whether text/uri-list is among the types.
    ** A source newer than our version speaks ours (the protocol says so), so its version is not cut off.
    */
    void X11Window::dndEnter(const XClientMessageEvent& message)
    {
        _dnd = DndState{};
        _dnd.source = static_cast<::Window>(message.data.l[0]);
        _dnd.version = static_cast<int>((static_cast<unsigned long>(message.data.l[1]) >> 24) & 0xFF);
        std::vector<Atom> types = dndOfferedTypes(message);
        _dnd.uriList = std::find(types.begin(), types.end(), atom("text/uri-list")) != types.end();
        logDndEnter(_native.display, _dnd, types);
    }

    /*
    ** Типы данных источника: до трёх — прямо в data.l[2..4]; больше (бит 0 в data.l[1]) — в свойстве
    ** XdndTypeList окна-источника. Размер свойства ограничен: длинный список типов не нужен для поиска одного.
    **
    ** The source data types: up to three are right in data.l[2..4]; more (bit 0 in data.l[1]) are in the
    ** XdndTypeList property of the source window. The property size is limited: a long type list is not needed to find one.
    */
    std::vector<Atom> X11Window::dndOfferedTypes(const XClientMessageEvent& message) const
    {
        std::vector<Atom> types;
        Atom type = 0;
        int format = 0;
        unsigned long count = 0;
        unsigned long remaining = 0;
        unsigned char* bytes = nullptr;

        if ((message.data.l[1] & 1) == 0)
        {
            for (int index = 2; index <= 4; ++index)
            {
                if (message.data.l[index] != 0)
                    types.push_back(static_cast<Atom>(message.data.l[index]));
            }
            return types;
        }
        int result = XGetWindowProperty(_native.display, static_cast<::Window>(message.data.l[0]), atom("XdndTypeList"),
            0, 256, False, XA_ATOM, &type, &format, &count, &remaining, &bytes);
        // Xlib отдаёт 32-битные элементы как long / Xlib returns 32-bit items as long
        const unsigned long* atoms = reinterpret_cast<const unsigned long*>(bytes);
        for (unsigned long index = 0; result == Success && format == 32 && atoms != nullptr && index < count; ++index)
            types.push_back(static_cast<Atom>(atoms[index]));
        if (bytes != nullptr)
            XFree(bytes);
        return types;
    }
}
