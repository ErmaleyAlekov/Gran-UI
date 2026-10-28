#include "gran/platform/win32/internal/drop_target.hpp"

#include <utility>

namespace gran::platform::win32
{
    /*
    ** Зоны запоминаются всегда; приёмник OLE регистрируется один раз, когда зоны впервые появились
    ** (окно без зон не принимает файлов — курсор «нельзя» даёт сама ОС). OleInitialize входит в тот же
    ** однопоточный апартамент, что и доступность; не удалось — окно просто не принимает файлов.
    **
    ** The zones are always kept; the OLE receiver is registered once, when zones first appear
    ** (a window without zones accepts no files — the OS itself gives the "no" cursor). OleInitialize enters the same
    ** single-threaded apartment as accessibility; on failure the window simply accepts no files.
    */
    void Win32Window::setDropZones(const std::vector<Rect>& zones)
    {
        dropZones = zones;
        if (zones.empty() || dropTarget || FAILED(OleInitialize(nullptr)))
            return;
        *dropTarget.put() = new DropTarget(*this);
        if (FAILED(RegisterDragDrop(window, dropTarget.get())))
        {
            dropTarget->detach();
            dropTarget.reset();
        }
    }

    // Самый ранний список путей; пусто — забирать нечего / The earliest path list; empty — nothing to take
    std::vector<std::string> Win32Window::takeDroppedFiles()
    {
        std::vector<std::string> paths;

        if (droppedFiles.empty())
            return paths;
        paths = std::move(droppedFiles.front());
        droppedFiles.pop_front();
        return paths;
    }

    // Из приёмника OLE: нехватка памяти отменяет бросок, а не бросает исключение в ОС / From the OLE receiver: out of memory cancels the drop instead of throwing into the OS
    bool Win32Window::queueDroppedFiles(std::vector<std::string>& paths) noexcept
    {
        try
        {
            droppedFiles.push_back(std::move(paths));
        }
        catch (...)
        {
            return false;
        }
        return true;
    }

    // Экран → клиентская область → логические пиксели / The screen → the client area → logical pixels
    Vec2 dropPoint(const Win32Window& window, POINTL point) noexcept
    {
        POINT client{point.x, point.y};

        ScreenToClient(window.window, &client);
        return Vec2{static_cast<float>(client.x) / window.currentScale, static_cast<float>(client.y) / window.currentScale};
    }

    // Копирование — только над зоной и только если источник его разрешает / Copying — only over a zone and only if the source allows it
    DWORD dropEffect(const Win32Window& window, Vec2 point, bool files, DWORD allowed) noexcept
    {
        if (!files || (allowed & DROPEFFECT_COPY) == 0)
            return DROPEFFECT_NONE;
        for (const Rect& zone : window.dropZones)
        {
            if (point.x >= zone.x && point.y >= zone.y && point.x < zone.x + zone.width && point.y < zone.y + zone.height)
                return DROPEFFECT_COPY;
        }
        return DROPEFFECT_NONE;
    }
}
