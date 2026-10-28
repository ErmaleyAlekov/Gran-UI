#ifndef GRAN_PLATFORM_WIN32_INTERNAL_DROP_TARGET_HPP
#define GRAN_PLATFORM_WIN32_INTERNAL_DROP_TARGET_HPP

#include "gran/platform/win32/win32_window.hpp"

#include <oleidl.h>

#include <atomic>
#include <string>
#include <vector>

namespace gran::platform::win32
{
    /*
    ** Приёмник перетаскивания OLE (IDropTarget) одного окна. ОС вызывает его в потоке окна
    ** во время разбора сообщений, в том числе посреди обработчика приложения (модальный диалог),
    ** поэтому он не трогает дерево UI: отвечает по зонам сброса окна (Window::setDropZones) и только
    ** складывает события в очередь окна. Методы noexcept: исключение не должно уйти в ОС.
    ** Время жизни — счётчик ссылок COM: окно держит одну ссылку и при уничтожении отвязывает приёмник.
    **
    ** The OLE drag-and-drop receiver (IDropTarget) of one window. The OS calls it in the window thread
    ** while dispatching messages, including in the middle of an application handler (a modal dialog),
    ** so it does not touch the UI tree: it answers by the window drop zones (Window::setDropZones) and only
    ** queues events into the window. The methods are noexcept: an exception must not escape into the OS.
    ** The lifetime is the COM reference count: the window holds one reference and detaches the receiver on destruction.
    */
    class DropTarget final : public IDropTarget
    {
    public:
        // ---- win32_drop_target.cpp ----
        explicit DropTarget(Win32Window& window) noexcept;
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** object) noexcept override;
        ULONG STDMETHODCALLTYPE AddRef() noexcept override;
        ULONG STDMETHODCALLTYPE Release() noexcept override;

        // Окно уничтожается: дальше на все запросы — «нельзя» / The window is being destroyed: from now on every request gets "no"
        void detach() noexcept;

        // ---- win32_drop_events.cpp ----
        HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* data, DWORD keys, POINTL point, DWORD* effect) noexcept override;
        HRESULT STDMETHODCALLTYPE DragOver(DWORD keys, POINTL point, DWORD* effect) noexcept override;
        HRESULT STDMETHODCALLTYPE DragLeave() noexcept override;
        HRESULT STDMETHODCALLTYPE Drop(IDataObject* data, DWORD keys, POINTL point, DWORD* effect) noexcept override;

    private:
        // Удаление — только через Release / Deletion — only through Release
        ~DropTarget() = default;

        std::atomic<ULONG> _references{1};
        Win32Window* _window;
        // Перетаскивают файлы (CF_HDROP), а не текст или что-то ещё / Files are dragged (CF_HDROP), not text or anything else
        bool _files = false;
    };

    // ---- win32_drop.cpp ----

    // Экранная точка ОС → логические пиксели клиентской области окна / An OS screen point → logical pixels of the window client area
    Vec2 dropPoint(const Win32Window& window, POINTL point) noexcept;

    // Ответ ОС: копирование над зоной сброса, если источник его разрешает, иначе «нельзя» / The answer to the OS: copying over a drop zone if the source allows it, otherwise "no"
    DWORD dropEffect(const Win32Window& window, Vec2 point, bool files, DWORD allowed) noexcept;

    // ---- win32_drop_data.cpp ----

    // Есть ли в данных список файлов CF_HDROP / Whether the data has a CF_HDROP file list
    bool hasFiles(IDataObject* data) noexcept;

    // Пути файлов из данных (UTF-8); false — данных нет или не хватило памяти / The file paths from the data (UTF-8); false — no data or out of memory
    bool readDroppedFiles(IDataObject* data, std::vector<std::string>& paths) noexcept;
}

#endif
