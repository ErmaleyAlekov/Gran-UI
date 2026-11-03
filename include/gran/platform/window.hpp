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

    /*
    ** Клавиши, которые различает движок (остальные — UNKNOWN): служебные, латинские буквы A–Z по физическому
    ** положению (Ctrl+C работает и в русской раскладке), цифры верхнего ряда, F1–F12, Insert и клавиша меню.
    ** Буквы, цифры и F1–F12 идут подряд: код вычисляется сдвигом от A, DIGIT_0 и F1.
    **
    ** The keys the engine distinguishes (the others are UNKNOWN): the service keys, the Latin letters A–Z by the physical
    ** position (Ctrl+C works in the Russian layout too), the top row digits, F1–F12, Insert and the menu key.
    ** The letters, the digits and F1–F12 go in a row: a code is computed as an offset from A, DIGIT_0 and F1.
    */
    enum class Key : std::uint8_t
    {
        UNKNOWN, BACKSPACE, TAB, ENTER, ESCAPE, SPACE, LEFT, RIGHT, UP, DOWN, HOME, END, DELETE_KEY,
        PAGE_UP, PAGE_DOWN, INSERT, CONTEXT_MENU,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        DIGIT_0, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4, DIGIT_5, DIGIT_6, DIGIT_7, DIGIT_8, DIGIT_9,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
    };

    // Имя клавиши для приложения ("A", "Enter", "ArrowLeft", "F5"; UNKNOWN — "Unidentified"), key_names.cpp
    // The key name for the application ("A", "Enter", "ArrowLeft", "F5"; UNKNOWN — "Unidentified"), key_names.cpp
    const char* keyName(Key key) noexcept;

    // Клавиша со сдвигом offset от первой в ряду (A, DIGIT_0, F1) / The key at offset from the first in a row (A, DIGIT_0, F1)
    Key keyAfter(Key first, unsigned offset) noexcept;

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

    class Window;

    /*
    ** Место окна в координатах ОС для настроек: Windows — обычная (не развёрнутая) рамка в пикселях устройства
    ** (rcNormalPosition), X11 — клиентская область относительно корня. Значения переносимы только в той же ОС.
    **
    ** The window place in OS coordinates for the settings: Windows — the normal (not maximized) frame in device pixels
    ** (rcNormalPosition), X11 — the client area relative to the root. The values are portable only within the same OS.
    */
    struct WindowPlacement
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool maximized = false;
    };

    /*
    ** Параметры окна (логические пиксели). owner — окно-владелец (невладеющая ссылка; живёт дольше
    ** подчинённого): подчинённое держится над ним и сворачивается вместе с ним. modal — подсказка ОС,
    ** что окно модальное (X11 _NET_WM_STATE_MODAL); сам ввод владельцу блокирует приложение (setEnabled).
    **
    ** The window parameters (logical pixels). owner is the owner window (a non-owning reference; it outlives
    ** the owned one): the owned window stays above it and minimizes together with it. modal is an OS hint
    ** that the window is modal (X11 _NET_WM_STATE_MODAL); the application blocks the owner input itself (setEnabled).
    */
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
        WindowKind kind = WindowKind::NORMAL;
        WindowPosition position = WindowPosition::DEFAULT;
        Window* owner = nullptr;
        bool modal = false;
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

        /*
        ** Окно-владелец модального окна выключается (Win32 EnableWindow): щелчок по нему ОС
        ** переводит на модальное. X11 такого не умеет — ввод отбрасывает приложение; по умолчанию ничего.
        **
        ** The owner window of a modal window is disabled (Win32 EnableWindow): the OS moves a click on it
        ** to the modal one. X11 cannot do that — the application drops the input; by default nothing.
        */
        virtual void setEnabled(bool enabled);

        // Поднять окно и отдать ему фокус (развернув из свёрнутого); по умолчанию ничего
        // Raise the window and give it the focus (restoring it from minimized); by default nothing
        virtual void activate();

        // Текущее место окна; false — ОС не умеет (по умолчанию) / The current window place; false — the OS cannot (by default)
        virtual bool placement(WindowPlacement& placement) const;

        /*
        ** Вернуть сохранённое место до показа окна; место вне всех мониторов не применяется (монитор отключили).
        ** Развёрнутость вступает в силу при show(). По умолчанию ничего.
        **
        ** Restore a saved place before showing the window; a place outside all the monitors is not applied (the monitor was unplugged).
        ** The maximization takes effect on show(). By default nothing.
        */
        virtual void setPlacement(const WindowPlacement& placement);

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
