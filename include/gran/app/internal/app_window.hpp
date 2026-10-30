#ifndef GRAN_APP_INTERNAL_APP_WINDOW_HPP
#define GRAN_APP_INTERNAL_APP_WINDOW_HPP

#include "gran/a11y/access_tree.hpp"
#include "gran/animation/animator.hpp"
#include "gran/app/application.hpp"
#include "gran/app/internal/web_slots.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/theme.hpp"
#include "gran/platform/window.hpp"
#include "gran/render/renderer.hpp"

#include <memory>
#include <vector>

namespace gran::app
{
    /*
    ** Одно окно приложения: свой документ разметки (тема, стили, дерево), окно ОС, бэкенд и рендерер,
    ** маршрутизатор ввода, дерево доступности, аниматор, браузеры <WebView> и подсказка. Общее у всех
    ** окон — в Application::State (shared): шрифты, действия, модели списков, задачи и таймеры.
    ** Окно — получатель действий и мест в тексте своего маршрутизатора (паттерн Mediator: ввод не знает
    ** ни приложения, ни рендерера).
    **
    ** Порядок полей важен: рендерер уничтожается раньше бэкенда (возвращает ему текстуры), бэкенд —
    ** раньше окна ОС (цепочка обмена ссылается на HWND), браузеры — раньше окна ОС (их окна-рамки — его дети),
    ** окно ОС — раньше дерева доступности (его провайдер читает дерево, пока окно не закрыто), всё —
    ** раньше документа (узлы).
    **
    ** One application window: its own markup document (the theme, the styles, the tree), the OS window, the backend and the renderer,
    ** the input router, the accessibility tree, the animator, the <WebView> browsers and the tooltip. What all the windows
    ** share is in Application::State (shared): the fonts, the actions, the list models, the tasks and the timers.
    ** The window receives the actions and the text places of its router (the Mediator pattern: input knows
    ** neither the application nor the renderer).
    **
    ** The field order matters: the renderer is destroyed before the backend (it returns the textures to it), the backend —
    ** before the OS window (the swap chain refers to the HWND), the browsers — before the OS window (their frame windows are its children),
    ** the OS window — before the accessibility tree (its provider reads the tree until the window closes), everything —
    ** before the document (the nodes).
    */
    struct AppWindow : input::ActionSink, input::TextLocator
    {
        // ---- app_window_list.cpp ----
        explicit AppWindow(Application::State& owner) noexcept;

        // ---- app_actions.cpp ----
        void action(const std::string& name, ui::Node& node) override;

        // Место в тексте узла по раскладке рендерера / A place in the node text by the renderer layout
        std::size_t offsetAt(const ui::Node& node, Vec2 point) override;

        // Края строки и шаги по строкам — по раскладке рендерера / The line edges and line steps — by the renderer layout
        std::size_t lineEdge(const ui::Node& node, std::size_t offset, bool end) override;
        std::size_t lineMove(const ui::Node& node, std::size_t offset, int lines, float& goalX) override;
        int pageLines(const ui::Node& node) override;

        // ---- app_window_events.cpp ----

        // Событие из разметки: обработчик onEvent, без него — действие onAction или gran:… / A markup event: the onEvent handler, without it — the onAction action or gran:…
        void event(const std::string& name, ui::Node& node, ui::UiEvent& event) override;

        // Общий счётчик удалений узлов всех окон / The shared node removal counter of all the windows
        std::uint32_t treeVersion() const noexcept override;

        // ---- app_commands.cpp ----

        // Горячая клавиша: команда этого окна или global другого; выполняется от узла фокуса (без него — корня)
        // A shortcut: a command of this window or a global one of another; it runs from the focus node (without it — the root)
        bool shortcut(const input::CommandQuery& query) override;

        // Кнопка или пункт меню command="id": выключенная команда — ничего, неизвестная — сообщение в stderr
        // A command="id" button or menu item: a disabled command — nothing, an unknown one — a message to stderr
        void command(const std::string& id, ui::Node& node) override;

        // Поле в фокусе или его история сменились: команды gran:undo / gran:redo основного стека пересчитываются
        // The focused field or its history changed: the gran:undo / gran:redo commands of the main stack are recomputed
        void editingChanged() override;

        // Общее состояние приложения / The shared application state
        Application::State& shared;
        markup::UiDocument document;
        style::ComputedStyle windowStyle;
        a11y::AccessTree access;
        std::unique_ptr<platform::Window> window;
        std::vector<std::unique_ptr<WebSlot>> webSlots;
        std::uint32_t webPass = 0;
        std::unique_ptr<rhi::RenderBackend> backend;
        std::unique_ptr<render::Renderer> renderer;
        std::unique_ptr<input::InputRouter> router;
        animation::Animator animator;
        // Фокус, запрошенный до создания маршрутизатора ввода / The focus requested before the input router exists
        ui::Node* pendingFocus = nullptr;
        // Зона onDrop, над которой сейчас несут файлы, и зоны, переданные окну ОС / The onDrop zone files are carried over now and the zones passed to the OS window
        ui::Node* dropNode = nullptr;
        std::vector<Rect> dropZones;
        // Окно просили закрыть (крестик, closeWindow): закрывается после прохода событий / The window was asked to close (the cross, closeWindow): it closes after the event pass
        bool closing = false;
        // Окну нужен кадр в этом проходе цикла / The window needs a frame in this loop pass
        bool changed = false;
        bool animating = false;
        bool caretVisible = true;
        double caretToggle = 0.0;
        // Каретка, к которой TextArea уже прокручено (узел, байт, длина текста, высота) / The caret a TextArea is already scrolled to (the node, the byte, the text length, the height)
        const ui::Node* revealNode = nullptr;
        std::size_t revealCaret = 0;
        std::size_t revealLength = 0;
        float revealHeight = 0.0f;
        // Подсказка tooltip: слой и его текст, узел-источник, время появления, показана, подавлена нажатием
        // The tooltip: the layer and its text, the source node, the appearance time, shown, suppressed by a press
        ui::Node* tooltip = nullptr;
        ui::Node* tooltipText = nullptr;
        const ui::Node* tooltipSource = nullptr;
        double tooltipTime = 0.0;
        bool tooltipShown = false;
        bool tooltipSuppressed = false;
    };

    // ---- application_frame.cpp ----

    // Стили, анимации, компоновка (если нужна), отрисовка / Styles, animations, layout (if needed), rendering
    void renderFrame(AppWindow& win);

    // ---- application_layout.cpp ----

    // Компоновка с подгонкой окон виртуальных списков (до трёх проходов) / A layout with fitting the virtual list windows (up to three passes)
    void layoutFrame(AppWindow& win, Size viewport);

    // ---- application_caret.cpp ----

    // Прокрутка TextArea в фокусе к каретке, если каретка, текст или высота изменились после компоновки
    // Scrolling the focused TextArea to the caret if the caret, the text or the height changed after layout
    void revealCaret(AppWindow& win);

    // ---- application_tooltip.cpp ----

    // Слой подсказки (Popup со стилем темы Tooltip) последним ребёнком корня / The tooltip layer (a Popup with the theme Tooltip style) as the last root child
    void createTooltip(AppWindow& win);

    // Скрыть подсказку; suppress — не показывать снова, пока мышь не перейдёт на другой узел / Hiding the tooltip; suppress — do not show it again until the mouse moves to another node
    void hideTooltip(AppWindow& win, bool suppress);

    // Смена узла под мышью и истечение задержки; true — подсказка показана или скрыта / A change of the node under the mouse and the delay expiry; true — the tooltip was shown or hidden
    bool updateTooltip(AppWindow& win, double time);

    // Сколько ждать до появления подсказки (отрицательное — ждать нечего) / How long to wait until the tooltip appears (negative — nothing to wait for)
    double tooltipWait(const AppWindow& win, double time) noexcept;

    // ---- application_graphics.cpp ----

    // Окно ОС по <Window>, бэкенд, рендерер, маршрутизатор ввода / The OS window by <Window>, the backend, the renderer, the input router
    void createGraphics(AppWindow& win);

    // Пересоздание бэкенда и рендерера после потери устройства / Recreating the backend and the renderer after a device loss
    void recreateGraphics(AppWindow& win);

    // ---- graphics_diagnostics.cpp ----

    // Сообщение отладочного слоя в отладочном режиме — исключение (живые окна тоже проверяются)
    // A debug layer message in debug mode is an exception (live windows are checked too)
    void checkGraphicsDiagnostics(AppWindow& win);

    // ---- application_window_frame.cpp ----

    // Свой заголовок окна спрашивает роль участка у маршрутизатора этого окна / The custom title bar asks this window router for the region role
    void connectWindowFrame(AppWindow& win);

    // Состояние Maximized из ОС — до каскада стилей кадра / The Maximized state from the OS — before the frame style cascade
    void syncWindowFrame(AppWindow& win);

    // ---- application_events.cpp ----

    // Событие окна; true — нужен кадр / A window event; true — a frame is needed
    bool handleEvent(AppWindow& win, const platform::Event& event);

    // Мигание каретки поля в фокусе; true — нужна перерисовка / The focused field caret blink; true — a repaint is needed
    bool blinkCaret(AppWindow& win, double time);

    // Сколько окну можно спать (отрицательное — без предела) / How long the window may sleep (negative — no limit)
    double windowSleep(const AppWindow& win, double time);

    // ---- file_drop.cpp ----

    // События перетаскивания файлов: подсветка зоны и доставка путей обработчику; true — нужен кадр / The file drag events: the zone highlight and delivering the paths to the handler; true — a frame is needed
    bool handleDrag(AppWindow& win, const platform::Event& event);

    // ---- drop_zones.cpp ----

    // После кадра: видимые зоны onDrop (с отсечением предков) — окну ОС, если изменились / After a frame: the visible onDrop zones (with the ancestor clipping) go to the OS window if they changed
    void syncDropZones(AppWindow& win);

    // ---- application_access.cpp ----

    // Выполнить запросы клиентов доступности; true — что-то изменилось (нужен кадр) / Perform the accessibility client requests; true — something changed (a frame is needed)
    bool runAccessRequests(AppWindow& win);
}

#endif
