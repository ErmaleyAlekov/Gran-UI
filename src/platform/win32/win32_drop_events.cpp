#include "gran/platform/win32/internal/drop_target.hpp"

namespace gran::platform::win32
{
    namespace
    {
        // Событие перетаскивания в точке / A drag event at a point
        Event dragEvent(EventKind kind, Vec2 point) noexcept
        {
            Event event;

            event.kind = kind;
            event.position = point;
            return event;
        }
    }

    /*
    ** Файлы внесли в окно: запоминаем, файлы ли это, отвечаем по зонам и сообщаем позицию
    ** (приложение подсветит зону). Не файлы — «нельзя» и без событий.
    **
    ** Files were brought into the window: we remember whether these are files, answer by the zones and report the position
    ** (the application highlights the zone). Not files — "no" and no events.
    */
    HRESULT STDMETHODCALLTYPE DropTarget::DragEnter(IDataObject* data, DWORD, POINTL point, DWORD* effect) noexcept
    {
        _files = _window != nullptr && hasFiles(data);
        return DragOver(0, point, effect);
    }

    // Файлы несут над окном / Files are carried over the window
    HRESULT STDMETHODCALLTYPE DropTarget::DragOver(DWORD, POINTL point, DWORD* effect) noexcept
    {
        Vec2 position;

        if (effect == nullptr)
            return E_INVALIDARG;
        if (_window == nullptr || !_files)
        {
            *effect = DROPEFFECT_NONE;
            return S_OK;
        }
        position = dropPoint(*_window, point);
        *effect = dropEffect(*_window, position, _files, *effect);
        _window->push(dragEvent(EventKind::DRAG_MOVE, position));
        return S_OK;
    }

    // Файлы унесли или перетаскивание отменили — подсветку снять / Files were carried away or dragging was cancelled — remove the highlight
    HRESULT STDMETHODCALLTYPE DropTarget::DragLeave() noexcept
    {
        if (_window != nullptr && _files)
            _window->push(dragEvent(EventKind::DRAG_LEAVE, Vec2{}));
        _files = false;
        return S_OK;
    }

    /*
    ** Бросили: над зоной — пути уходят в очередь окна вместе с событием DROP, иначе (или если
    ** пути не прочитались) — DRAG_LEAVE, чтобы снять подсветку.
    **
    ** Dropped: over a zone — the paths go into the window queue together with the DROP event, otherwise (or if
    ** the paths could not be read) — DRAG_LEAVE to remove the highlight.
    */
    HRESULT STDMETHODCALLTYPE DropTarget::Drop(IDataObject* data, DWORD, POINTL point, DWORD* effect) noexcept
    {
        std::vector<std::string> paths;
        Vec2 position;

        if (effect == nullptr)
            return E_INVALIDARG;
        if (_window == nullptr || !_files)
        {
            *effect = DROPEFFECT_NONE;
            return S_OK;
        }
        position = dropPoint(*_window, point);
        *effect = dropEffect(*_window, position, _files, *effect);
        _files = false;
        if (*effect == DROPEFFECT_NONE || !readDroppedFiles(data, paths) || !_window->queueDroppedFiles(paths))
        {
            *effect = DROPEFFECT_NONE;
            _window->push(dragEvent(EventKind::DRAG_LEAVE, position));
            return S_OK;
        }
        _window->push(dragEvent(EventKind::DROP, position));
        return S_OK;
    }
}
