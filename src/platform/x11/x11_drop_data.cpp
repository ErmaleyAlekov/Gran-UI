#include "gran/platform/x11/x11_window.hpp"
#include "gran/utils/uri_list.hpp"
#include <string_view>
#include <utility>

namespace gran::platform::x11
{
    // Наибольший список путей в байтах (как у буфера обмена): больше — бросок не принимается
    // The largest path list in bytes (as for the clipboard): a larger one — the drop is not accepted
    static const long DND_MAX_BYTES = 4194304;

    /*
    ** Данные броска пришли (SelectionNotify на XdndSelection). Пути — в очередь, затем событие DROP: очередь
    ** путей идёт парой с событиями DROP (Window::takeDroppedFiles). Ни одного локального файла или отказ
    ** владельца — XdndFinished «не выполнено» и DRAG_LEAVE. Ответ без запроса (сеанса нет) пропускается.
    **
    ** The drop data came (SelectionNotify on XdndSelection). The paths go into the queue, then the DROP event: the path
    ** queue goes in pairs with the DROP events (Window::takeDroppedFiles). Not a single local file or an owner refusal —
    ** XdndFinished "not done" and DRAG_LEAVE. An answer without a request (no session) is skipped.
    */
    void X11Window::dndData(const XSelectionEvent& event)
    {
        std::vector<std::string> paths;

        if (!_dnd.converting)
            return;
        if (event.property != None)
            paths = dndReadPaths(event.property);
        else
            logDndData(_native.display, None, 0, 0, 0);
        if (paths.empty())
        {
            dndReply("XdndFinished", 0, nullptr);
            dndEnd(EventKind::DRAG_LEAVE);
            return;
        }
        _dropped.push_back(std::move(paths));
        dndReply("XdndFinished", 1, "XdndActionCopy");
        dndEnd(EventKind::DROP);
    }

    /*
    ** Свойство с текстом text/uri-list читается целиком и удаляется; тип, формат (8 бит) и полнота
    ** (remaining == 0, без INCR) проверяются до разбора.
    **
    ** The property with the text/uri-list text is read whole and deleted; the type, the format (8 bits) and the
    ** completeness (remaining == 0, no INCR) are checked before parsing.
    */
    std::vector<std::string> X11Window::dndReadPaths(Atom property)
    {
        Atom type = 0;
        int format = 0;
        unsigned long count = 0;
        unsigned long remaining = 0;
        unsigned char* bytes = nullptr;
        std::vector<std::string> paths;

        int result = XGetWindowProperty(_native.display, _native.window, property, 0, DND_MAX_BYTES / 4, True,
            AnyPropertyType, &type, &format, &count, &remaining, &bytes);
        if (result == Success && type == atom("text/uri-list") && format == 8 && remaining == 0 && bytes != nullptr)
            paths = parseUriList(std::string_view(reinterpret_cast<const char*>(bytes), count));
        logDndData(_native.display, type, format, count, paths.size());
        if (bytes != nullptr)
            XFree(bytes);
        return paths;
    }
}
