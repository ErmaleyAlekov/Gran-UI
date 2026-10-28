#ifndef GRAN_PLATFORM_WINDOW_HPP
#define GRAN_PLATFORM_WINDOW_HPP

#include "gran/utils/geometry.hpp"
#include "gran/platform/window_frame.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace gran::a11y
{
    struct AccessTree;
}

namespace gran::platform
{
    /*
    ** Вид события окна. Перетаскивание файлов: DRAG_MOVE — файлы несут над окном (позиция),
    ** DRAG_LEAVE — унесли или отменили, DROP — бросили (позиция; пути — Window::takeDroppedFiles).
    **
    ** The window event kind. Dragging files: DRAG_MOVE — files are carried over the window (the position),
    ** DRAG_LEAVE — carried away or cancelled, DROP — dropped (the position; the paths — Window::takeDroppedFiles).
    */
    enum class EventKind : std::uint8_t
    {
        CLOSE, RESIZE, DPI_CHANGED, EXPOSE, MOUSE_MOVE, MOUSE_DOWN, MOUSE_UP, MOUSE_WHEEL, MOUSE_LEAVE,
        KEY_DOWN, KEY_UP, CHARACTER, FOCUS_GAINED, FOCUS_LOST, DRAG_MOVE, DRAG_LEAVE, DROP
    };

    // Кнопка мыши / A mouse button
    enum class MouseButton : std::uint8_t { LEFT, RIGHT, MIDDLE };

    // Клавиши, которые движок обрабатывает сам (остальные — UNKNOWN) / The keys the engine handles itself (the others are UNKNOWN)
    enum class Key : std::uint8_t
    {
        UNKNOWN, BACKSPACE, TAB, ENTER, ESCAPE, SPACE, LEFT, RIGHT, UP, DOWN, HOME, END, DELETE_KEY,
        PAGE_UP, PAGE_DOWN, A, C, V, X, Z
    };

    // Модификаторы клавиатуры (биты) / The keyboard modifiers (bits)
    enum Modifier : std::uint8_t
    {
        MODIFIER_SHIFT = 1,
        MODIFIER_CONTROL = 2,
        MODIFIER_ALT = 4
    };

    /*
    ** Событие окна. Координаты мыши — в логических пикселях клиентской области (уже
    ** поделены на масштаб DPI); размер при RESIZE — в пикселях устройства.
    **
    ** A window event. The mouse coordinates are in logical pixels of the client area (already
    ** divided by the DPI scale); the size on RESIZE is in device pixels.
    */
    struct Event
    {
        EventKind kind = EventKind::EXPOSE;
        Vec2 position;
        Vec2 wheel;
        MouseButton button = MouseButton::LEFT;
        Key key = Key::UNKNOWN;
        std::uint8_t modifiers = 0;
        // Число щелчков подряд при MOUSE_DOWN: 2 — двойной щелчок / The click count on MOUSE_DOWN: 2 — a double click
        std::uint8_t clicks = 1;
        char32_t character = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        float scale = 1.0f;
    };

    // Курсор мыши (порядок — как style::Cursor) / The mouse cursor (the order is as style::Cursor)
    enum class CursorShape : std::uint8_t { ARROW, HAND, TEXT, MOVE, NOT_ALLOWED, GRAB, RESIZE_EW, RESIZE_NS };

    // Параметры окна (логические пиксели) / The window parameters (logical pixels)
    struct WindowDesc
    {
        std::string title;
        float width = 800.0f;
        float height = 600.0f;
        float minWidth = 0.0f;
        float minHeight = 0.0f;
        bool resizable = true;
        bool darkTitleBar = false;
        TitleBar titleBar = TitleBar::SYSTEM;
    };

    /*
    ** Окно ОС (паттерн Bridge: приложение не знает, Win32 это, X11 или Cocoa). Оконная
    ** процедура только складывает события в очередь; исключение — обратный вызов перерисовки
    ** при перетаскивании края окна (иначе содержимое «замирает»), он обёрнут в noexcept-барьер.
    **
    ** An OS window (the Bridge pattern: the application does not know whether it is Win32, X11 or Cocoa).
    ** The window procedure only queues events; the exception is the repaint callback while dragging
    ** the window edge (otherwise the content "freezes"), it is wrapped into a noexcept barrier.
    */
    class Window
    {
    public:
        virtual ~Window() = default;

        // Следующее событие без ожидания; false — очередь пуста / The next event without waiting; false — the queue is empty
        virtual bool pollEvent(Event& event) = 0;

        // Ждать событий не дольше timeout секунд (отрицательное — без предела) / Wait for events at most timeout seconds (negative — no limit)
        virtual void waitEvents(double timeout) = 0;

        // HWND / Window для бэкенда отрисовки / The HWND / Window for the render backend
        virtual void* nativeHandle() const = 0;

        // Соединение с оконной системой для Vulkan; у Windows не требуется.
        // The window system connection for Vulkan; not required on Windows.
        virtual void* nativeDisplay() const;

        // Масштаб DPI (1 — 96 dpi) / The DPI scale (1 — 96 dpi)
        virtual float scale() const = 0;

        // Размер клиентской области в пикселях устройства / The client area size in device pixels
        virtual Size pixelSize() const = 0;

        // Курсор над окном / The cursor over the window
        virtual void setCursor(CursorShape shape) = 0;

        // Показ окна после первого кадра (без белой вспышки) / Showing the window after the first frame (without a white flash)
        virtual void show() = 0;

        // Управление окном в потоке UI; close посылает обычное событие закрытия.
        // Window management on the UI thread; close sends the usual close event.
        virtual void minimize();
        virtual void maximize();
        virtual void restore();
        virtual void close();
        virtual bool isMaximized() const;

        // Чтение роли под мышью в потоке UI; пустой обработчик отключает связь с деревом.
        // Read the role under the pointer on the UI thread; an empty handler disconnects the tree.
        virtual void setFrameHitTest(std::function<WindowRole(Vec2)> callback);

        // Обратный вызов перерисовки во время изменения размера мышью / The repaint callback while resizing with the mouse
        virtual void setLiveResize(std::function<void()> callback) = 0;

        // Текст буфера обмена / The clipboard text
        virtual std::string clipboardText() = 0;
        virtual void setClipboardText(const std::string& text) = 0;

        /*
        ** Области окна (логические пиксели), принимающие файлы. Пусто — окно файлов не принимает
        ** (курсор «нельзя»). Окно отвечает ОС по этим областям само, не обращаясь к дереву UI:
        ** запрос ОС может прийти посреди обработчика (модальный диалог, изменение размера).
        **
        ** The window areas (logical pixels) that accept files. Empty — the window accepts no files
        ** (the "no" cursor). The window answers the OS by these areas itself without touching the UI tree:
        ** an OS request may come in the middle of a handler (a modal dialog, resizing).
        ** По умолчанию (window_defaults.cpp) окно файлов не принимает. / By default (window_defaults.cpp) the window accepts no files.
        */
        virtual void setDropZones(const std::vector<Rect>& zones);

        // Пути файлов самого раннего ещё не забранного события DROP (UTF-8) / The file paths of the earliest DROP event not taken yet (UTF-8)
        virtual std::vector<std::string> takeDroppedFiles();

        // Дерево доступности для вспомогательных технологий ОС (nullptr — отключить) / The accessibility tree for the OS assistive technologies (nullptr — disconnect)
        virtual void setAccessTree(a11y::AccessTree* tree) = 0;

        // Фокус в дереве доступности сменился — сообщить дикторам / The accessibility tree focus changed — notify the screen readers
        virtual void accessFocusChanged() = 0;

        // Сверить состояние узла в фокусе с прошлым кадром и сообщить дикторам об изменении (отметка, текст, число)
        // Compare the focused node state with the previous frame and notify the screen readers of a change (the mark, the text, the number)
        virtual void accessRefresh() = 0;

        /*
        ** Разбудить waitEvents из любого потока (единственный потокобезопасный метод окна):
        ** поток интерфейса проснётся и заберёт задачи, отправленные другими потоками.
        **
        ** Wakes waitEvents from any thread (the only thread-safe window method):
        ** the UI thread wakes up and takes the tasks posted by other threads.
        */
        virtual void wake() noexcept = 0;
    };

    // Окно текущей ОС / A window of the current OS
    std::unique_ptr<Window> createWindow(const WindowDesc& desc);
}

#endif
