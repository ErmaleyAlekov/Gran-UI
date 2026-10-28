#ifndef GRAN_APP_INTERNAL_APP_STATE_HPP
#define GRAN_APP_INTERNAL_APP_STATE_HPP

#include "gran/app/application.hpp"
#include "gran/app/internal/app_window.hpp"

#include <map>
#include <mutex>
#include <vector>

namespace gran
{
    /*
    ** Состояние приложения — общее для всех окон. Порядок полей важен: окна объявлены после шрифтов
    ** (их рендереры ссылаются на библиотеку шрифтов) и до привязок списков (привязки ссылаются на узлы
    ** окон и снимаются раньше документов).
    **
    ** The application state shared by all the windows. The field order matters: the windows are declared after the fonts
    ** (their renderers refer to the font library) and before the list bindings (the bindings refer to the window nodes
    ** and are dropped before the documents).
    */
    struct Application::State : binding::RemovalListener
    {
        // ---- node_removal.cpp ----

        // Снять ссылки ввода, анимаций и доступности на поддерево перед его удалением / Drop the input, animation and accessibility references to the subtree before removing it
        void nodeRemoving(const ui::Node& node) noexcept override;

        AppOptions options;
        text::FontLibrary fonts;
        std::map<std::string, ActionHandler> actions;
        // Окна; первое — главное: его закрытие завершает run() / The windows; the first is the main one: closing it ends run()
        std::vector<std::unique_ptr<app::AppWindow>> windows;
        // Модели списков и привязки items="…" всех окон; слушатель удаления — само состояние
        // The list models and the items="…" bindings of all the windows; the removal listener is the state itself
        binding::ListBindings lists{this};
        std::vector<app::WebMessage> webInbox;
        std::map<std::string, WebMessageHandler> webHandlers;
        std::map<std::string, FileDropHandler> dropHandlers;
        // Задачи других потоков (post) под замком; wakeTarget — главное окно, пока идёт run() / The tasks of other threads (post) under the lock; wakeTarget — the main window while run() goes on
        std::mutex taskMutex;
        std::vector<Task> tasks;
        platform::Window* wakeTarget = nullptr;
        // Таймеры setTimeout/setInterval (только поток интерфейса) / The setTimeout/setInterval timers (the UI thread only)
        app::TimerQueue timers;
        bool quit = false;
    };

    namespace app
    {
        // ---- application_load.cpp ----

        /*
        ** Принять загруженный документ окну: начальные состояния, стиль окна, шрифты, данные и привязки списков.
        ** Привязки прежнего документа этого окна снимаются.
        **
        ** Adopt a loaded document into a window: the initial states, the window style, the fonts, the data and the list bindings.
        ** The bindings of the former document of this window are dropped.
        */
        void adoptDocument(AppWindow& win, markup::UiDocument document, const std::string& base);

        // ---- application_frame.cpp ----

        // Секунды монотонных часов / Seconds of the monotonic clock
        double now();

        // ---- application_windows.cpp ----

        // Окно узла (по корню дерева) или nullptr, если узел не в окне / The node window (by the tree root) or nullptr if the node is not in a window
        AppWindow* findWindow(Application::State& state, const ui::Node& node) noexcept;

        // Маршрутизатор ввода окна узла (nullptr — узел не в окне или окно ещё не показано) / The input router of the node window (nullptr — the node is not in a window or the window is not shown yet)
        input::InputRouter* routerOf(Application::State& state, const ui::Node& node) noexcept;

        // ---- app_window_list.cpp ----

        // Окно, которому принадлежит узел (по корню дерева); узел вне окон — главное окно / The window the node belongs to (by the tree root); a node outside the windows — the main window
        AppWindow& windowOf(Application::State& state, const ui::Node& node);

        // Новое окно с документом; графику и первый кадр даёт openWindows / A new window with a document; openWindows gives the graphics and the first frame
        AppWindow& addWindow(Application::State& state, markup::UiDocument document, const std::string& base);

        // Окнам без графики — окно ОС, бэкенд, первый кадр и показ (из цикла run) / The windows without graphics get the OS window, the backend, the first frame and showing (from the run loop)
        void openWindows(Application::State& state);

        // Закрыть окна с флагом closing; закрытие главного — выход из run() / Close the windows with the closing flag; closing the main one — leaving run()
        void closeWindows(Application::State& state);

        // ---- application_tasks.cpp ----

        /*
        ** Время run(): задачи других потоков будят главное окно.
        ** При выходе (и по исключению) получатель снимается до того, как State разрушит окна.
        **
        ** The run() time: the tasks of other threads wake the main window.
        ** On leaving (and on an exception) the target is removed before State destroys the windows.
        */
        struct RunScope
        {
            explicit RunScope(Application::State& owner);
            ~RunScope();
            RunScope(const RunScope&) = delete;
            RunScope& operator=(const RunScope&) = delete;

            Application::State& state;
        };

        // Выполнить накопленные задачи; true — были задачи (нужен кадр) / Run the accumulated tasks; true — there were tasks (a frame is needed)
        bool runTasks(Application::State& state);

        // Окно, которое будит post (nullptr — не будить: окна уже/ещё нет) / The window that post wakes (nullptr — do not wake: no window yet/anymore)
        void setWakeTarget(Application::State& state, platform::Window* window);
    }
}

#endif
