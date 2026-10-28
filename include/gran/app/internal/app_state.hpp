#ifndef GRAN_APP_INTERNAL_APP_STATE_HPP
#define GRAN_APP_INTERNAL_APP_STATE_HPP

#include "gran/a11y/access_tree.hpp"
#include "gran/animation/animator.hpp"
#include "gran/app/application.hpp"
#include "gran/app/internal/web_slots.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/theme.hpp"
#include "gran/platform/window.hpp"
#include "gran/render/renderer.hpp"

#include <map>
#include <mutex>
#include <vector>

namespace gran
{
    /*
    ** Состояние приложения. Порядок полей важен: рендерер уничтожается раньше бэкенда
    ** (возвращает ему текстуры), бэкенд — раньше окна (цепочка обмена ссылается на HWND),
    ** окно — раньше дерева доступности (его провайдер читает дерево, пока окно не закрыто).
    **
    ** The application state. The field order matters: the renderer is destroyed before the backend
    ** (it returns the textures to it), the backend — before the window (the swap chain refers to the HWND),
    ** the window — before the accessibility tree (its provider reads the tree until the window closes).
    */
    struct Application::State : input::ActionSink, input::TextLocator, binding::RemovalListener
    {
        // ---- node_removal.cpp ----

        // Снять ссылки ввода, анимаций и доступности на поддерево перед его удалением / Drop the input, animation and accessibility references to the subtree before removing it
        void nodeRemoving(const ui::Node& node) noexcept override;

        // ---- app_actions.cpp ----
        void action(const std::string& name, ui::Node& node) override;

        // Место в тексте узла по раскладке рендерера / A place in the node text by the renderer layout
        std::size_t offsetAt(const ui::Node& node, Vec2 point) override;

        // Края строки и шаги по строкам — по раскладке рендерера / The line edges and line steps — by the renderer layout
        std::size_t lineEdge(const ui::Node& node, std::size_t offset, bool end) override;
        std::size_t lineMove(const ui::Node& node, std::size_t offset, int lines, float& goalX) override;
        int pageLines(const ui::Node& node) override;

        AppOptions options;
        markup::UiDocument document;
        style::ComputedStyle windowStyle;
        text::FontLibrary fonts;
        std::map<std::string, ActionHandler> actions;
        // Модели списков и привязки items="…"; слушатель удаления — само состояние / The list models and the items="…" bindings; the removal listener is the state itself
        binding::ListBindings lists{this};
        a11y::AccessTree access;
        std::unique_ptr<platform::Window> window;
        // Встроенные браузеры <WebView> — после окна: закрываются раньше него (их окна-рамки — дети окна)
        // The embedded <WebView> browsers — after the window: they close before it (their frame windows are window children)
        std::vector<std::unique_ptr<app::WebSlot>> webSlots;
        std::vector<app::WebMessage> webInbox;
        std::map<std::string, WebMessageHandler> webHandlers;
        std::uint32_t webPass = 0;
        std::unique_ptr<rhi::RenderBackend> backend;
        std::unique_ptr<render::Renderer> renderer;
        std::unique_ptr<input::InputRouter> router;
        animation::Animator animator;
        // Задачи других потоков (post) под замком; wakeTarget — окно, пока идёт run() / The tasks of other threads (post) under the lock; wakeTarget — the window while run() goes on
        std::mutex taskMutex;
        std::vector<Task> tasks;
        platform::Window* wakeTarget = nullptr;
        // Таймеры setTimeout/setInterval (только поток интерфейса) / The setTimeout/setInterval timers (the UI thread only)
        app::TimerQueue timers;
        // Фокус, запрошенный до создания маршрутизатора ввода / The focus requested before the input router exists
        ui::Node* pendingFocus = nullptr;
        bool quit = false;
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

    namespace app
    {
        // ---- application_load.cpp ----

        // Принять загруженный документ: начальные состояния, стиль окна, шрифты, данные и привязки списков
        // Adopt a loaded document: the initial states, the window style, the fonts, the data and the list bindings
        void adoptDocument(Application::State& state, markup::UiDocument document, const std::string& base);

        // ---- application_frame.cpp ----

        // Секунды монотонных часов / Seconds of the monotonic clock
        double now();

        // Стили, анимации, компоновка (если нужна), отрисовка / Styles, animations, layout (if needed), rendering
        void renderFrame(Application::State& state);

        // ---- application_layout.cpp ----

        // Компоновка с подгонкой окон виртуальных списков (до трёх проходов) / A layout with fitting the virtual list windows (up to three passes)
        void layoutFrame(Application::State& state, Size viewport);

        // ---- application_caret.cpp ----

        // Прокрутка TextArea в фокусе к каретке, если каретка, текст или высота изменились после компоновки
        // Scrolling the focused TextArea to the caret if the caret, the text or the height changed after layout
        void revealCaret(Application::State& state);

        // ---- application_tooltip.cpp ----

        // Слой подсказки (Popup со стилем темы Tooltip) последним ребёнком корня / The tooltip layer (a Popup with the theme Tooltip style) as the last root child
        void createTooltip(Application::State& state);

        // Скрыть подсказку; suppress — не показывать снова, пока мышь не перейдёт на другой узел / Hiding the tooltip; suppress — do not show it again until the mouse moves to another node
        void hideTooltip(Application::State& state, bool suppress);

        // Смена узла под мышью и истечение задержки; true — подсказка показана или скрыта / A change of the node under the mouse and the delay expiry; true — the tooltip was shown or hidden
        bool updateTooltip(Application::State& state, double time);

        // Сколько ждать до появления подсказки (отрицательное — ждать нечего) / How long to wait until the tooltip appears (negative — nothing to wait for)
        double tooltipWait(const Application::State& state, double time) noexcept;

        // ---- application_graphics.cpp ----

        // Окно, бэкенд D3D12 и рендерер / The window, the D3D12 backend and the renderer
        void createGraphics(Application::State& state);

        // Пересоздание бэкенда и рендерера после потери устройства / Recreating the backend and the renderer after a device loss
        void recreateGraphics(Application::State& state);

        // Событие окна; true — нужен кадр / A window event; true — a frame is needed
        bool handleEvent(Application::State& state, const platform::Event& event);

        // ---- application_tasks.cpp ----

        // Выполнить накопленные задачи; true — были задачи (нужен кадр) / Run the accumulated tasks; true — there were tasks (a frame is needed)
        bool runTasks(Application::State& state);

        // Окно, которое будит post (nullptr — не будить: окна уже/ещё нет) / The window that post wakes (nullptr — do not wake: no window yet/anymore)
        void setWakeTarget(Application::State& state, platform::Window* window);
    }
}

#endif
