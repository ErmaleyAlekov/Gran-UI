#ifndef GRAN_APP_INTERNAL_APP_STATE_HPP
#define GRAN_APP_INTERNAL_APP_STATE_HPP

#include "gran/app/application.hpp"
#include "gran/app/internal/app_window.hpp"
#include "gran/app/internal/task_pool.hpp"

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

        // Наблюдаемый файл: путь, время записи и размер при последней проверке / A watched file: the path, the write time and the size at the last check
        struct FileStamp
        {
            std::string path;
            std::filesystem::file_time_type time{};
            std::uintmax_t size = 0;
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

        // ---- window_closing.cpp ----

        // Графика окон уходит с конца списка (подчинённые раньше владельцев) / The window graphics go from the list end (the owned ones before the owners)
        ~State() override;

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
        // Файлы библиотек компонентов главного окна: их правка тоже перезагружает разметку / The component library files of the main window: editing them reloads the markup too
        std::vector<app::FileStamp> libraryStamps;
        double nextReloadCheck = 0.0;
        // Идёт run(); идёт замена документа (рисовать нельзя) / run() goes on; a document replacement goes on (no rendering)
        bool running = false;
        bool reloading = false;
        /*
        ** Пул фоновых задач (создаётся при первой runTask). Последнее поле — уничтожается первым:
        ** его потоки дожидаются, пока очередь post (taskMutex, tasks) ещё жива.
        **
        ** The background task pool (created on the first runTask). The last field — destroyed first:
        ** its threads are waited for while the post queue (taskMutex, tasks) is still alive.
        */
        // Настройки программы (application_settings.cpp) / The program settings (application_settings.cpp)
        settings::Settings settings;
        // Отчёт о сбое прошлого запуска (application_logging.cpp) / The crash report of the previous run (application_logging.cpp)
        std::string previousCrash;
        std::unique_ptr<app::TaskPool> pool;
    };

    namespace app
    {
        // ---- reload_libraries.cpp ----

        // Изменился ли какой-нибудь файл библиотеки (отметки обновляются у всех) / Whether any library file changed (the stamps of all are updated)
        bool librariesChanged(Application::State& state);

        // Запомнить файлы библиотек документа и их нынешние отметки / Remember the document library files and their current stamps
        void rememberLibraries(Application::State& state, const markup::UiDocument& document);

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

        // ---- reload_patch.cpp ----

        /*
        ** Частичная замена документа окна документом next той же разметки (live_patch): равные ветви
        ** остаются, изменённые перестраиваются на месте. false — пролог или каталог различаются (или
        ** ветвь не строится), окно не тронуто — нужна полная замена.
        **
        ** A partial replacement of the window document by the document next of the same markup (live_patch): the equal branches
        ** stay, the changed ones are rebuilt in place. false — the prologue or the directory differ (or
        ** a branch does not build), the window is untouched — a full replacement is needed.
        */
        bool patchDocument(AppWindow& win, markup::UiDocument& next);

        // ---- node_rebuild.cpp ----

        /*
        ** Узел node получает содержимое поддерева fresh, оставаясь тем же объектом (родитель, слушатели,
        ** наведение и фокус сохраняются); прежние потомки снимаются со всех ссылок окна и уничтожаются.
        ** Состояние потомков (и самого узла при keepOwnState) переходит на новые узлы, фокус — тоже.
        **
        ** The node gets the content of the fresh subtree while staying the same object (the parent, the listeners,
        ** the hover and the focus are kept); the former descendants are dropped from all the window references and destroyed.
        ** The descendants state (and the node own one with keepOwnState) moves to the new nodes, the focus too.
        */
        void rebuildInPlace(AppWindow& win, ui::Node& node, std::unique_ptr<ui::Node> fresh, bool keepOwnState);

        // Проверить файлы <Font> до замены дерева: испорченный шрифт оставляет прежний интерфейс
        // Check the <Font> files before replacing the tree: a broken font keeps the former interface
        void validateFonts(const markup::UiDocument& document, const std::string& base);

        // ---- application_frame.cpp ----

        // Секунды монотонных часов / Seconds of the monotonic clock
        double now();

        // ---- window_lookup.cpp ----

        // Окно узла (по корню дерева) или nullptr, если узел не в окне / The node window (by the tree root) or nullptr if the node is not in a window
        AppWindow* findWindow(Application::State& state, const ui::Node& node) noexcept;

        // Маршрутизатор ввода окна узла (nullptr — узел не в окне или окно ещё не показано) / The input router of the node window (nullptr — the node is not in a window or the window is not shown yet)
        input::InputRouter* routerOf(Application::State& state, const ui::Node& node) noexcept;

        // ---- window_owner.cpp ----

        // Пометить окно узла к закрытию с результатом для onClosed / Mark the node window for closing with a result for onClosed
        void markClosing(Application::State& state, const ui::Node& node, const std::string& result);

        // Окно-владелец из WindowOptions (nullptr — без владельца); узел вне окон — GranException
        // The owner window from WindowOptions (nullptr — without an owner); a node outside the windows is GranException
        AppWindow* requireOwner(Application::State& state, const WindowOptions& options);

        // Владелец, модальность и обработчик закрытия нового окна / The owner, the modality and the close handler of a new window
        void attachOwner(AppWindow& win, AppWindow* owner, const WindowOptions& options);

        // Вид, место, модальность и окно ОС владельца для описания нового окна ОС / The kind, the place, the modality and the owner OS window for describing a new OS window
        void ownWindow(const AppWindow& win, platform::WindowDesc& desc);

        // ---- window_modal.cpp ----

        // Верхнее модальное окно, блокирующее win (nullptr — win свободно) / The top modal window blocking win (nullptr — win is free)
        AppWindow* blockingModal(Application::State& state, const AppWindow& win) noexcept;

        // Пересчитать blocked всех окон, выключить или включить их окна ОС / Recompute blocked of all the windows, disable or enable their OS windows
        void updateBlocking(Application::State& state);

        // Ввод заблокированного окна отбросить, модальное поднять; true — событие поглощено
        // Drop the input of a blocked window, raise the modal one; true — the event is consumed
        bool dropBlockedInput(AppWindow& win, const platform::Event& event);

        // ---- app_window_list.cpp ----

        // Окно, которому принадлежит узел (по корню дерева); узел вне окон — главное окно / The window the node belongs to (by the tree root); a node outside the windows — the main window
        AppWindow& windowOf(Application::State& state, const ui::Node& node);

        // Новое окно с документом; графику и первый кадр даёт openWindows / A new window with a document; openWindows gives the graphics and the first frame
        AppWindow& addWindow(Application::State& state, markup::UiDocument document, const std::string& base);

        // Окнам без графики — окно ОС, бэкенд, первый кадр и показ (из цикла run) / The windows without graphics get the OS window, the backend, the first frame and showing (from the run loop)
        void openWindows(Application::State& state);

        // ---- application_logging.cpp ----

        // Журнал и обработчик сбоя по AppOptions::appName (из конструктора приложения) / The log and the crash handler by AppOptions::appName (from the application constructor)
        void startLogging(Application::State& state);

        // Видеокарта и драйвер окна — в журнал / The window video card and driver go into the log
        void logAdapter(const AppWindow& win);

        // ---- application_settings.cpp ----

        // Загрузить файл настроек по AppOptions (из конструктора) / Load the settings file by AppOptions (from the constructor)
        void startSettings(Application::State& state);

        // Состояние всех окон — в настройки, файл — на диск (конец run) / The state of all the windows into the settings, the file to disk (the end of run)
        void saveSettings(Application::State& state);

        // ---- window_layout.cpp ----

        // Место окна из настроек — до создания бэкенда / The window place from the settings — before the backend creation
        void restorePlacement(AppWindow& win);

        // Атрибуты persist и переменные persist из настроек — до первого кадра / The persist attributes and variables from the settings — before the first frame
        void restoreState(AppWindow& win);

        // Место окна, атрибуты persist, переменные persist — в настройки / The window place, the persist attributes, the persist variables into the settings
        void saveState(AppWindow& win);

        // ---- window_closing.cpp ----

        // Закрыть окна с флагом closing (с подчинёнными, onClosed); закрытие главного — выход из run()
        // Close the windows with the closing flag (with the owned ones, onClosed); closing the main one — leaving run()
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
