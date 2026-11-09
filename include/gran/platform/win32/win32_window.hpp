#ifndef GRAN_PLATFORM_WIN32_WIN32_WINDOW_HPP
#define GRAN_PLATFORM_WIN32_WIN32_WINDOW_HPP

#include "gran/platform/window.hpp"

// Широкие (UTF-16) версии API и макросов (IDC_*, LoadCursor) / The wide (UTF-16) API and macro versions (IDC_*, LoadCursor)
// Без макросов min/max из <windows.h>: они ломают std::min/std::max (MSVC) / Without the min/max macros of <windows.h>: they break std::min/std::max (MSVC)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

#include "gran/rhi/dx12/internal/com_ref.hpp"

#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace gran::platform::win32
{
    // Связь окна с деревом доступности (internal/uia_element.hpp) / The window link to the accessibility tree (internal/uia_element.hpp)
    struct UiaHost;

    // Приёмник перетаскивания OLE (internal/drop_target.hpp) / The OLE drag-and-drop receiver (internal/drop_target.hpp)
    class DropTarget;

    /*
    ** Окно Win32 с поддержкой DPI per-monitor v2. Оконная процедура (handleMessage) noexcept:
    ** события складываются в очередь, исключения внутри неё гасятся — они не должны
    ** пересекать границу ОС (правило gpu_rules.md, п. 3).
    **
    ** A Win32 window with per-monitor v2 DPI support. The window procedure (handleMessage) is noexcept:
    ** events are queued, exceptions inside it are swallowed — they must not cross
    ** the OS boundary (gpu_rules.md, item 3).
    */
    class Win32Window final : public Window
    {
    public:
        // ---- win32_window.cpp ----
        explicit Win32Window(const WindowDesc& desc);
        ~Win32Window() override;
        void* nativeHandle() const override;

        // ---- win32_events.cpp ----
        bool pollEvent(Event& event) override;
        void waitEvents(double timeout) override;
        Size pixelSize() const override;
        void setLiveResize(std::function<void()> callback) override;

        // ---- win32_system.cpp ----
        float scale() const override;
        bool keyboardCuesAlways() const override;
        void accessAnnounce(const ui::Node& node) override;
        void show() override;
        std::string clipboardText() override;
        void setClipboardText(const std::string& text) override;

        // ---- win32_cursor.cpp ----
        void setCursor(CursorShape shape) override;

        // ---- win32_commands.cpp ----
        void minimize() override;
        void maximize() override;
        void restore() override;
        void close() override;
        bool isMaximized() const override;

        // ---- win32_owner.cpp ----
        void setEnabled(bool enabled) override;
        void activate() override;

        // ---- win32_placement.cpp ----
        bool placement(WindowPlacement& placement) const override;
        void setPlacement(const WindowPlacement& placement) override;

        // ---- win32_frame.cpp ----
        void setFrameHitTest(std::function<WindowRole(Vec2)> callback) override;

        // ---- win32_access.cpp ----
        void setAccessTree(a11y::AccessTree* tree) override;
        void accessFocusChanged() override;
        void accessRefresh() override;

        // ---- win32_wake.cpp ----
        void wake() noexcept override;

        // ---- win32_drop.cpp ----
        void setDropZones(const std::vector<Rect>& zones) override;
        std::vector<std::string> takeDroppedFiles() override;

        // Пути брошенных файлов в очередь (из приёмника OLE); false — не хватило памяти / The dropped file paths into the queue (from the OLE receiver); false — out of memory
        bool queueDroppedFiles(std::vector<std::string>& paths) noexcept;

        // ---- win32_procedure.cpp ----
        // Обработка сообщения ОС (вызывается оконной процедурой) / Handling an OS message (called by the window procedure)
        LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) noexcept;

        // ---- win32_events.cpp ----
        // Событие в очередь (нехватка памяти — событие теряется, а не исключение) / An event into the queue (out of memory — the event is lost, not an exception)
        void push(const Event& event) noexcept;

        // Состояние для обработчиков сообщений / The state for the message handlers
        HWND window = nullptr;
        float currentScale = 1.0f;
        bool sizing = false;
        bool trackingLeave = false;
        bool trackingNonClient = false;
        char16_t highSurrogate = 0;
        CursorShape cursor = CursorShape::ARROW;
        // Код зоны из последнего WM_SETCURSOR: курсор интерфейса ставится только над HTCLIENT,
        // иначе кадр перебивал бы курсор растягивания у края окна.
        // The area code of the last WM_SETCURSOR: the interface cursor is set only over HTCLIENT,
        // otherwise a frame would override the resize cursor at the window edge.
        WORD cursorArea = HTCLIENT;
        std::function<void()> liveResize;
        WindowDesc desc;
        // Показать развёрнутым (место из настроек, setPlacement) / Show maximized (the place from the settings, setPlacement)
        bool startMaximized = false;
        // Обработчик читает дерево только в UI-потоке, не хранит указатели на узлы.
        // The callback reads the tree only on the UI thread and keeps no node pointers.
        std::function<WindowRole(Vec2)> frameHitTest;
        // Провайдер UI Automation создаётся при первом setAccessTree / The UI Automation provider is created on the first setAccessTree
        std::shared_ptr<UiaHost> access;
        // Зоны сброса файлов (логические пиксели) и пути брошенных файлов по событиям DROP / The file drop zones (logical pixels) and the dropped file paths by the DROP events
        std::vector<Rect> dropZones;
        std::deque<std::vector<std::string>> droppedFiles;
        // Приёмник OLE регистрируется при первой непустой зоне / The OLE receiver is registered on the first non-empty zone
        rhi::dx12::ComRef<DropTarget> dropTarget;

    private:
        std::deque<Event> _events;
    };

    // ---- win32_input.cpp: перевод сообщений ввода / translating the input messages ----

    // Сообщения мыши; false — это не сообщение мыши / Mouse messages; false — it is not a mouse message
    bool translateMouse(Win32Window& window, UINT message, WPARAM wParam, LPARAM lParam);

    // Своя рамка: сообщения, геометрия клиента, проверка краёв и мышь кнопки Snap.
    // Custom frame: messages, client geometry, edge hit testing and the Snap button pointer.
    bool handleFrameMessage(Win32Window& window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result) noexcept;
    bool translateFrameMouse(Win32Window& window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result) noexcept;
    LRESULT hitFrame(Win32Window& window, LPARAM point);

    // Ручка размера окна с системной рамкой: HTBOTTOMRIGHT над ней, иначе ответ системы / The size grip of a system-framed window: HTBOTTOMRIGHT over it, otherwise the system answer
    LRESULT hitGrip(Win32Window& window, LPARAM point, LRESULT system) noexcept;
    void configureFrame(Win32Window& window) noexcept;
    void fitClientRect(const Win32Window& window, RECT& frame) noexcept;
    void setInitialSize(Win32Window& window, DWORD style) noexcept;

    // ---- win32_paint.cpp ----

    // WM_ERASEBKGND: клиентская область — сплошным фоном окна / WM_ERASEBKGND: the client area — with the solid window background
    void paintBackground(const Win32Window& window, HDC dc) noexcept;

    // WM_PAINT: стереть фон (BeginPaint шлёт WM_ERASEBKGND) и попросить кадр / WM_PAINT: erase the background (BeginPaint sends WM_ERASEBKGND) and ask for a frame
    void handlePaint(Win32Window& window) noexcept;

    // ---- win32_owner.cpp ----

    // Стиль окна по виду: обычное (WS_OVERLAPPEDWINDOW), диалог и инструментальное — без свёртки и развёртки
    // The window style by the kind: a normal one (WS_OVERLAPPEDWINDOW), a dialog and a tool one — without minimize and maximize
    DWORD windowStyle(const WindowDesc& desc) noexcept;

    // Расширенный стиль: рамка диалога без значка, узкий заголовок инструментального окна
    // The extended style: a dialog frame without an icon, the narrow title bar of a tool window
    DWORD windowExStyle(const WindowDesc& desc) noexcept;

    // ---- win32_placement.cpp ----

    // Поставить скрытое окно по desc.position: по центру владельца или монитора / Place the hidden window by desc.position: at the owner or monitor center
    void placeWindow(Win32Window& window, HWND owner) noexcept;

    // ---- win32_cursor.cpp ----

    // WM_SETCURSOR: запоминает зону; true — курсор интерфейса поставлен (зона клиента своего окна).
    // WM_SETCURSOR: remembers the area; true — the interface cursor is set (the client area of the own window).
    bool handleSetCursor(Win32Window& window, WPARAM wParam, LPARAM lParam) noexcept;

    // ---- win32_keyboard.cpp ----

    // Сообщения клавиатуры и символов; false — не они / Keyboard and character messages; false — not them
    bool translateKeyboard(Win32Window& window, UINT message, WPARAM wParam);

    // Нажатые модификаторы (биты Modifier) / The pressed modifiers (Modifier bits)
    std::uint8_t currentModifiers() noexcept;

    // WM_SYSCOMMAND/SC_KEYMENU кроме Alt+пробел: поглотить (нет сигнала и режима системного меню) / WM_SYSCOMMAND/SC_KEYMENU except Alt+space: consume it (no beep and no system menu mode)
    bool swallowKeyMenu(UINT message, WPARAM wParam, LPARAM lParam) noexcept;

    // ---- win32_access.cpp ----

    // Ответ на WM_GETOBJECT (корень UI Automation) / The answer to WM_GETOBJECT (the UI Automation root)
    LRESULT answerAccessRequest(Win32Window& window, WPARAM wParam, LPARAM lParam) noexcept;

    // Отключение провайдеров перед уничтожением окна / Disconnecting the providers before the window is destroyed
    void disconnectAccess(Win32Window& window) noexcept;

    // ---- win32_text.cpp: строки UTF-8 ↔ UTF-16 / UTF-8 ↔ UTF-16 strings ----

    // UTF-8 → UTF-16 / UTF-8 → UTF-16
    std::wstring toWide(const std::string& text);

    // UTF-16 → UTF-8 / UTF-16 → UTF-8
    std::string toUtf8(const wchar_t* text);

    // Однократная настройка процесса: DPI per-monitor v2 / The one-time process setup: DPI per-monitor v2
    void enableDpiAwareness() noexcept;
}

#endif
