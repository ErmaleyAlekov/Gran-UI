#ifndef GRAN_PLATFORM_HEADLESS_WINDOW_HPP
#define GRAN_PLATFORM_HEADLESS_WINDOW_HPP

#include "gran/platform/window.hpp"

#include <condition_variable>
#include <deque>
#include <mutex>

namespace gran::platform
{
    /*
    ** Видимое снаружи состояние безоконного окна: что с ним сделало приложение. Тест читает его через
    ** HeadlessWindow::state() и проверяет, например, курсор над кнопкой или развёрнутость.
    **
    ** The state of a windowless window visible from outside: what the application did to it. A test reads it via
    ** HeadlessWindow::state() and checks, for example, the cursor over a button or the maximization.
    */
    struct HeadlessState
    {
        CursorShape cursor = CursorShape::ARROW;
        bool visible = false;
        bool enabled = true;
        bool maximized = false;
        bool minimized = false;
        unsigned activations = 0;
        WindowPlacement placement;
        std::vector<Rect> dropZones;
        a11y::AccessTree* accessTree = nullptr;
        unsigned focusChanges = 0;
        unsigned announcements = 0;
    };

    /*
    ** Окно без ОС (третья реализация контракта platform::, шаг M2): события подаёт тест (pushEvent),
    ** ОС ничего не рисует — nativeHandle() равен nullptr, поэтому бэкенд отрисовки создаётся вне экрана
    ** (кадр читается из памяти) или пустой (rhi::NullBackend, без GPU). Так тесты интерфейса идут на любой
    ** машине, в том числе на сборочных серверах без экрана и видеокарты. Буфер обмена — общий для всех
    ** безоконных окон процесса (копирование в одном окне, вставка в другом — как в настоящей ОС).
    **
    ** A window without an OS (the third implementation of the platform:: contract, step M2): a test feeds the events
    ** (pushEvent), the OS draws nothing — nativeHandle() is nullptr, so the render backend is created offscreen
    ** (the frame is read from memory) or empty (rhi::NullBackend, without a GPU). So the UI tests run on any
    ** machine, including build servers without a screen and a graphics card. The clipboard is shared by all the
    ** windowless windows of the process (copy in one window, paste in another — as in a real OS).
    */
    class HeadlessWindow final : public Window
    {
    public:
        // ---- headless_events.cpp ----

        explicit HeadlessWindow(const WindowDesc& desc = WindowDesc{});

        // Положить событие в очередь; можно из любого потока / Put an event into the queue; allowed from any thread
        void pushEvent(const Event& event);

        bool pollEvent(Event& event) override;
        void waitEvents(double timeout) override;
        void wake() noexcept override;

        // ---- headless_window.cpp ----

        void* nativeHandle() const override;
        float scale() const override;
        Size pixelSize() const override;
        void show() override;
        void setLiveResize(std::function<void()> callback) override;

        // ---- headless_commands.cpp ----

        void minimize() override;
        void maximize() override;
        void restore() override;
        void close() override;
        bool isMaximized() const override;

        // ---- headless_state.cpp ----

        void setCursor(CursorShape shape) override;
        void setEnabled(bool enabled) override;
        void activate() override;
        bool placement(WindowPlacement& placement) const override;
        void setPlacement(const WindowPlacement& placement) override;

        // ---- headless_system.cpp ----

        std::string clipboardText() override;
        void setClipboardText(const std::string& text) override;
        void setDropZones(const std::vector<Rect>& zones) override;
        std::vector<std::string> takeDroppedFiles() override;

        // ---- headless_access.cpp ----

        void setAccessTree(a11y::AccessTree* tree) override;
        void accessFocusChanged() override;
        void accessRefresh() override;
        void accessAnnounce(const ui::Node& node) override;

        // Что приложение сделало с окном / What the application did to the window
        const HeadlessState& state() const noexcept;

        // ---- headless_test_input.cpp ----

        // Новый размер в логических пикселях — событие RESIZE в пикселях устройства / A new size in logical pixels — a RESIZE event in device pixels
        void resizeTo(float width, float height);

        // Окно перешло на монитор с другим масштабом: DPI_CHANGED, затем RESIZE / The window moved to a monitor with another scale: DPI_CHANGED, then RESIZE
        void changeScale(float scale);

        // Файлы бросили в точку окна: DRAG_MOVE и DROP; пути забирает takeDroppedFiles / Files are dropped at a window point: DRAG_MOVE and DROP; takeDroppedFiles takes the paths
        void dropFiles(Vec2 position, const std::vector<std::string>& paths);

        // ---- headless_frame.cpp ----

        void setFrameHitTest(std::function<WindowRole(Vec2)> callback) override;

        // Роль участка рамки под точкой (как спросила бы ОС); NONE — обработчика нет / The frame region role under a point (as the OS would ask); NONE — no handler
        WindowRole frameRole(Vec2 point) const;

        // Вызвать перерисовку «во время перетаскивания края» / Call the repaint "while dragging the edge"
        void liveResize();

    private:
        // Поля защищены _mutex только там, где их трогает другой поток: очередь и флаг пробуждения.
        // The fields are guarded by _mutex only where another thread touches them: the queue and the wake flag.
        WindowDesc _desc;
        float _scale = 1.0f;
        float _width = 0.0f;
        float _height = 0.0f;
        HeadlessState _state;
        std::deque<Event> _events;
        std::deque<std::vector<std::string>> _drops;
        std::function<void()> _liveResize;
        std::function<WindowRole(Vec2)> _frameHitTest;
        mutable std::mutex _mutex;
        std::condition_variable _signal;
        bool _woken = false;
    };
}

#endif
