#ifndef GRAN_APP_INTERNAL_APP_STATE_HPP
#define GRAN_APP_INTERNAL_APP_STATE_HPP

#include "gran/app/application.hpp"
#include "gran/app/internal/app_window.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace gran
{
    namespace app
    {
        /*
        ** Именованный стек отмены приложения и его наблюдатель (паттерн Observer): после каждой смены
        ** стека включаются и выключаются команды gran:undo[:имя] / gran:redo[:имя] всех окон и вызывается
        ** обработчик onUndoChanged. Стек — поле связи, поэтому наблюдатель не переживает свой стек.
        **
        ** A named application undo stack and its observer (the Observer pattern): after every stack change
        ** the gran:undo[:name] / gran:redo[:name] commands of all the windows are enabled and disabled and
        ** the onUndoChanged handler is called. The stack is a field of the link, so the observer does not outlive its stack.
        */
        class UndoLink final : public undo::UndoObserver
        {
        public:
            // ---- undo_link.cpp ----
            UndoLink(Application::State& state, std::string name);
            void undoChanged(const undo::UndoStack& changed) override;

            undo::UndoStack stack;

        private:
            Application::State& _state;
            std::string _name;
        };
    }

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
        // enabled команд, заданные кодом (setCommandEnabled): переживают перезагрузку и действуют на окна, открытые позже
        // The command enabled values set by the code (setCommandEnabled): they survive a reload and apply to the windows opened later
        std::map<std::string, bool> commandStates;
        // Стеки отмены приложения по имени ("" — основной) и обработчик их смены / The application undo stacks by name ("" is the main one) and their change handler
        std::map<std::string, std::unique_ptr<app::UndoLink>> undoStacks;
        UndoHandler undoHandler;
        // Именованные обработчики событий разметки (onEvent) / The named markup event handlers (onEvent)
        std::map<std::string, EventHandler> eventHandlers;
        // Номер последнего слушателя кода; счётчик удалений узлов для доставки событий (ActionSink::treeVersion)
        // The last code listener number; the node removal counter for the event delivery (ActionSink::treeVersion)
        ListenerId lastListener = 0;
        std::uint32_t treeVersion = 0;
        // Задачи других потоков (post) под замком; wakeTarget — главное окно, пока идёт run() / The tasks of other threads (post) under the lock; wakeTarget — the main window while run() goes on
        std::mutex taskMutex;
        std::vector<Task> tasks;
        platform::Window* wakeTarget = nullptr;
        // Таймеры setTimeout/setInterval (только поток интерфейса) / The setTimeout/setInterval timers (the UI thread only)
        app::TimerQueue timers;
        bool quit = false;
        // Невладеющая ссылка для встроенных команд gran:… / A non-owning reference for the built-in gran:… commands
        Application* owner = nullptr;
        // Перезагрузка главного окна: документ ждёт границы цикла, чтобы обработчик не удалил свой узел.
        // Reloading the main window: the document waits for a loop boundary so a handler does not delete its own node.
        std::unique_ptr<markup::UiDocument> pendingDocument;
        std::string pendingPath;
        // Наблюдаемый файл главного окна (пусто — разметка из строки) / The watched main window file (empty — markup from a string)
        std::string sourcePath;
        std::string reloadFailure;
        std::filesystem::file_time_type sourceTime{};
        std::uintmax_t sourceSize = 0;
        double nextReloadCheck = 0.0;
        // Идёт run(); идёт замена документа (рисовать нельзя) / run() goes on; a document replacement goes on (no rendering)
        bool running = false;
        bool reloading = false;
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

        // ---- app_commands.cpp ----

        // Команда по id: своего окна (любая область), затем global других окон; nullptr — нет такой
        // A command by id: of the own window (any scope), then the global ones of the other windows; nullptr — none
        ui::Command* findCommand(Application::State& state, AppWindow& own, const std::string& id);

        // Выполнить действие команды (action или id) от узла node / Run the command action (action or id) from the node
        void runCommand(AppWindow& win, const ui::Command& command, ui::Node& node);

        // ---- command_binding.cpp ----

        // enabled команд нового документа — из заданных кодом / The enabled values of the new document commands — from the ones set by the code
        void applyCommandStates(AppWindow& win);

        /*
        ** Узлы поддерева (и шаблоны списков) с command: enabled команды, её подпись (если своей нет) и подпись
        ** первого сочетания. Неизвестная команда узел не меняет.
        **
        ** The subtree nodes (and the list templates) with command: the command enabled, its label (if there is no own one) and the label
        ** of the first combination. An unknown command does not change the node.
        */
        void bindCommands(AppWindow& win, ui::Node& subtree);

        // После смены enabled команды id — её узлы во всех окнах / After the command id enabled changes — its nodes in all the windows
        void refreshCommand(Application::State& state, const std::string& id);

        // ---- undo_commands.cpp ----

        // Команды gran:undo / gran:redo окна следуют за своим стеком (стека ещё нет — выключены) / The gran:undo / gran:redo window commands follow their stack (no stack yet — disabled)
        void linkUndoCommands(Application::State& state, AppWindow& win);

        // После смены стека name — его команды и их узлы во всех окнах / After the name stack changes — its commands and their nodes in all the windows
        void refreshUndoCommands(Application::State& state, const std::string& name);

        // ---- application_reload.cpp, reload_document.cpp, reload_source.cpp, reload_fonts.cpp ----

        /*
        ** Документ главного окна: первый создаёт окно, следующий ждёт границы цикла (внутри run)
        ** или заменяет сразу. path — абсолютный путь файла, пусто — разметка из строки.
        **
        ** The main window document: the first one creates the window, the next one waits for a loop boundary (inside run)
        ** or replaces at once. path — the absolute file path, empty — markup from a string.
        */
        void stageDocument(Application::State& state, markup::UiDocument document, const std::string& path);

        // Применить отложенный документ; true — заменён (нужен кадр) / Apply the pending document; true — replaced (a frame is needed)
        bool applyReload(Application::State& state);

        // Наблюдение за файлом раз в 250 мс; ошибка разметки сохраняет прежнее дерево / Watching the file every 250 ms; a markup error keeps the former tree
        bool pollReload(Application::State& state);

        // Сколько ждать до проверки файла (отрицательное — наблюдения нет) / How long to wait until the file check (negative — no watching)
        double reloadWait(const Application::State& state, double time) noexcept;

        // Файл изменился (время или размер); запоминает новые значения / The file changed (the time or the size); remembers the new values
        bool sourceChanged(Application::State& state);

        // Запомнить источник главного окна / Remember the main window source
        void rememberSource(Application::State& state, const std::string& path);

        // Каталог разметки для относительных путей шрифтов и картинок / The markup directory for relative font and image paths
        std::string sourceBase(const std::string& path);

        // Проверить файлы <Font> до замены дерева: испорченный шрифт оставляет прежний интерфейс
        // Check the <Font> files before replacing the tree: a broken font keeps the former interface
        void validateFonts(const markup::UiDocument& document, const std::string& base);

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
