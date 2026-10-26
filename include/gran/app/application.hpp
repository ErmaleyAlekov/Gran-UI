#ifndef GRAN_APP_APPLICATION_HPP
#define GRAN_APP_APPLICATION_HPP

#include "gran/ui/node.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace gran
{
    // Обработчик действия разметки (onClick="имя") / A markup action handler (onClick="name")
    using ActionHandler = std::function<void(ui::Node& node)>;

    // Задача для потока интерфейса (Application::post) / A task for the UI thread (Application::post)
    using Task = std::function<void()>;

    // Параметры приложения / The application parameters
    struct AppOptions
    {
        bool debugLayer = false;
        bool vsync = true;
        double exitAfter = 0.0;   // секунд до выхода (0 — не выходить; для дымовых проверок) / seconds until exit (0 — never; for smoke checks)
    };

    /*
    ** Приложение Gran UI (паттерн Facade): разметка → окно → цикл событий. Кадр рисуется только
    ** при изменениях (ввод, анимация, каретка, размер окна); в простое поток спит в ожидании ОС.
    **
    ** A Gran UI application (the Facade pattern): markup → a window → the event loop. A frame is drawn only
    ** on changes (input, an animation, the caret, the window size); when idle the thread sleeps waiting for the OS.
    */
    class Application
    {
    public:
        // ---- application.cpp ----
        explicit Application(AppOptions options = AppOptions{});
        ~Application();
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        // Разметка из файла (проверка схемой; ошибка — MarkupException) / Markup from a file (schema checked; an error is MarkupException)
        void loadFile(const std::string& path);

        // Разметка из строки / Markup from a string
        void loadString(std::string_view markup);

        // Обработчик действия по имени / An action handler by name
        void onAction(const std::string& name, ActionHandler handler);

        // ---- application_frame.cpp ----

        // Узел по id (nullptr — нет такого) / A node by id (nullptr — no such node)
        ui::Node* find(const std::string& id);

        // Сменить именованный стиль узла (<Style name>); нет такого — GranException(RESOURCE) / Change the node named style (<Style name>); no such style — GranException(RESOURCE)
        void setStyle(ui::Node& node, const std::string& styleName);

        // ---- application_tree.cpp: правка дерева во время работы (только поток интерфейса) ----
        // ---- application_tree.cpp: editing the tree at run time (the UI thread only) ----

        // Вставить узел последним ребёнком (обычно копию шаблона ui::cloneTree) / Insert a node as the last child (usually a template copy ui::cloneTree)
        ui::Node& append(ui::Node& parent, std::unique_ptr<ui::Node> child);

        /*
        ** Удалить узел с поддеревом: ссылки ввода (фокус, наведение, выделение), анимации
        ** и фокус доступности на его узлы снимаются; корень удалить нельзя (GranException).
        **
        ** Remove a node with its subtree: the input references (focus, hover, selection), animations
        ** and the accessibility focus on its nodes are dropped; the root cannot be removed (GranException).
        */
        void remove(ui::Node& node);

        // Фокус ввода из кода (nullptr — снять); до run() — запоминается / The input focus from code (nullptr — remove); before run() — remembered
        void focus(ui::Node* node);

        // ---- application_popups.cpp: всплывающие слои (<Popup>), только во время run() / popup layers (<Popup>), only during run() ----

        // Показать слой под узлом anchor (над ним, если внизу нет места) — выпадающий список / Show the layer under the anchor node (over it if there is no room below) — a drop-down list
        void showPopup(ui::Node& popup, const ui::Node& anchor);

        // Показать слой в точке окна (логические пиксели) — контекстное меню / Show the layer at a window point (logical pixels) — a context menu
        void showPopupAt(ui::Node& popup, Vec2 point);

        // Скрыть слой (Esc, щелчок мимо и выбор пункта меню скрывают его сами) / Hide the layer (Esc, a click outside and choosing a menu item hide it themselves)
        void hidePopup(ui::Node& popup);

        // ---- application_tasks.cpp ----

        /*
        ** Задача из любого потока: выполнится в потоке интерфейса на ближайшем проходе цикла
        ** (окно будится), после неё кадр перерисуется. Так фоновые потоки (сеть, файлы)
        ** меняют дерево, не трогая его сами. Задачи, не успевшие выполниться до выхода
        ** из run(), уничтожаются вместе с приложением без вызова.
        **
        ** A task from any thread: it runs in the UI thread on the nearest loop pass
        ** (the window is woken), the frame is redrawn after it. This way background threads (network, files)
        ** change the tree without touching it themselves. Tasks that have not run before leaving
        ** run() are destroyed together with the application without being called.
        */
        void post(Task task);

        // ---- application_run.cpp ----

        // Окно и цикл событий до закрытия; возвращает код выхода / The window and the event loop until closing; returns the exit code
        int run();

        struct State;

    private:
        std::unique_ptr<State> _state;
    };
}

#endif
