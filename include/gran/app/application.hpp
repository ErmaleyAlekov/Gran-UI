#ifndef GRAN_APP_APPLICATION_HPP
#define GRAN_APP_APPLICATION_HPP

#include "gran/app/timer_queue.hpp"
#include "gran/binding/list_bindings.hpp"
#include "gran/platform/file_dialog.hpp"
#include "gran/ui/node.hpp"
#include "gran/rhi/render_backend.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gran
{
    // Обработчик действия разметки (onClick="имя") / A markup action handler (onClick="name")
    using ActionHandler = std::function<void(ui::Node& node)>;

    /*
    ** Событие интерфейса и его обработчик (onPointerDown="имя", addEventListener): узел, чей обработчик вызван,
    ** и событие — его флаги stopPropagation/preventDefault видят остальные обработчики и движок.
    **
    ** A UI event and its handler (onPointerDown="name", addEventListener): the node whose handler is called
    ** and the event — its stopPropagation/preventDefault flags are seen by the other handlers and the engine.
    */
    using UiEvent = ui::UiEvent;
    using EventHandler = ui::EventHandler;
    using ListenerId = ui::ListenerId;

    // Обработчик строки из страницы <WebView onMessage="имя"> / A handler of a string from a <WebView onMessage="name"> page
    using WebMessageHandler = std::function<void(ui::Node& webView, const std::string& message)>;

    // Обработчик файлов, брошенных на элемент <… onDrop="имя">: пути в UTF-8 / A handler of files dropped onto an <… onDrop="name"> element: the paths in UTF-8
    using FileDropHandler = std::function<void(ui::Node& node, const std::vector<std::string>& files)>;

    // Задача для потока интерфейса (Application::post) / A task for the UI thread (Application::post)
    using Task = std::function<void()>;

    // Запись списка, к которой относится узел (Application::itemOf); model == nullptr — узел не из списка
    // The list record a node belongs to (Application::itemOf); model == nullptr — the node is not from a list
    using ItemRef = binding::ItemRef;

    // Тип файлов и параметры системного диалога файлов / A file type and the system file dialog parameters
    using FileFilter = platform::FileFilter;
    using FileDialogOptions = platform::FileDialogOptions;

    // Номер таймера (Application::setTimeout/setInterval); 0 — «нет таймера» / A timer number (Application::setTimeout/setInterval); 0 — "no timer"
    using TimerId = app::TimerId;

    // Параметры приложения / The application parameters
    struct AppOptions
    {
        bool debugLayer = false;
        bool vsync = true;
        double exitAfter = 0.0;   // секунд до выхода (0 — не выходить; для дымовых проверок) / seconds until exit (0 — never; for smoke checks)
        rhi::BackendKind backend = rhi::BackendKind::AUTO;
        bool forceWarp = false;
        bool watchMarkup = false; // проверять XML каждые 250 мс / check XML every 250 ms
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

        /*
        ** Перезагрузить последний файл; внутри run замена откладывается до границы цикла.
        ** Ошибка разбора сохраняет старое дерево. После замены повторите find: старые узлы
        ** удалены. Обработчики, таймеры и внешние модели сохраняются, данные XML загружаются снова.
        **
        ** Reload the last file; inside run replacement is deferred to a loop boundary.
        ** A parse error preserves the old tree. After replacement call find again: old nodes
        ** are deleted. Handlers, timers and external models remain; XML data is loaded again.
        */
        void reload();

        // Загрузить изменившийся файл; без файла — false. Только поток UI.
        // Load a changed file; without a file returns false. UI thread only.
        bool reloadIfChanged();

        // Последняя ошибка наблюдения за файлом; успех очищает строку.
        // The latest file watcher error; success clears the string.
        const std::string& reloadError() const;

        // Обработчик действия по имени / An action handler by name
        void onAction(const std::string& name, ActionHandler handler);

        // ---- app_event_handlers.cpp: события интерфейса (только поток интерфейса) ----
        // ---- app_event_handlers.cpp: UI events (the UI thread only) ----

        /*
        ** Обработчик событий разметки по имени: onKeyDown="name", onPointerMove="name", on="своё:name"…
        ** Узел обработчика — тот, у кого атрибут (event.current), цель — event.target. Без onEvent имя
        ** ищется среди onAction (обработчик получает только узел) и встроенных gran:….
        **
        ** A markup event handler by name: onKeyDown="name", onPointerMove="name", on="custom:name"…
        ** The handler node is the one with the attribute (event.current), the target is event.target. Without onEvent the name
        ** is looked for among onAction (the handler gets the node only) and the built-in gran:….
        */
        void onEvent(const std::string& name, EventHandler handler);

        /*
        ** Слушатель события type ("pointerDown", "keyUp", своё имя) у узла: capture — фаза захвата.
        ** Слушатель живёт в узле: удаление узла снимает его. Номер — для removeEventListener.
        **
        ** A listener of the type event ("pointerDown", "keyUp", a custom name) at the node: capture — the capture phase.
        ** The listener lives in the node: removing the node drops it. The number is for removeEventListener.
        */
        ListenerId addEventListener(ui::Node& node, const std::string& type, EventHandler handler, bool capture = false);

        // Снять слушателя (можно из него самого); чужой или неизвестный номер — ничего / Remove a listener (also from itself); a foreign or an unknown number — nothing
        void removeEventListener(ui::Node& node, ListenerId id);

        // ---- app_event_dispatch.cpp ----

        /*
        ** Своё событие из кода: захват → цель → всплытие (bubbles), обработчики разметки on="type:…" и слушатели.
        ** detail — строка для обработчиков. Встроенное имя (pointerDown) даёт событие этого вида без действий
        ** движка. true — никто не вызвал preventDefault.
        **
        ** A custom event from code: capture → the target → bubbling (bubbles), the on="type:…" markup handlers and the listeners.
        ** detail is a string for the handlers. A built-in name (pointerDown) gives an event of this kind without the engine
        ** actions. true — nobody called preventDefault.
        */
        bool dispatchEvent(ui::Node& target, const std::string& type, const std::string& detail = std::string(),
            bool bubbles = true);

        /*
        ** Захват указателя (как setPointerCapture): события мыши идут узлу, где бы ни была мышь, до отпускания
        ** кнопки или releasePointer. До run() и для узла вне окна — ничего.
        **
        ** The pointer capture (as setPointerCapture): the mouse events go to the node wherever the mouse is until the button
        ** release or releasePointer. Before run() and for a node outside a window — nothing.
        */
        void capturePointer(ui::Node& node);

        // Отпустить захват, если его держит node / Release the capture if node holds it
        void releasePointer(ui::Node& node);

        // ---- application_windows.cpp: несколько окон (только поток интерфейса) ----
        // ---- application_windows.cpp: several windows (the UI thread only) ----

        /*
        ** Ещё одно окно из файла разметки: своя тема, стили, данные и дерево; действия (onAction),
        ** модели списков, таймеры и задачи — общие со всеми окнами. Возвращает корень содержимого
        ** окна. Окно появляется на ближайшем проходе цикла run() (до run() — вместе с главным).
        ** Первое загруженное окно (loadFile, loadString или openWindow) — главное: его закрытие завершает run().
        **
        ** One more window from a markup file: its own theme, styles, data and tree; the actions (onAction),
        ** the list models, the timers and the tasks are shared by all the windows. Returns the window content
        ** root. The window appears on the nearest run() loop pass (before run() — together with the main one).
        ** The first loaded window (loadFile, loadString or openWindow) is the main one: closing it ends run().
        */
        ui::Node& openWindow(const std::string& path);

        // То же из строки разметки / The same from a markup string
        ui::Node& openWindowString(std::string_view markup);

        /*
        ** Закрыть окно, которому принадлежит узел (после текущего прохода событий; узлы окна
        ** после этого недействительны). Главное окно — выход из run(). Узел не из окна — ничего.
        **
        ** Close the window the node belongs to (after the current event pass; the window nodes
        ** are invalid after that). The main window — leaving run(). A node not from a window — nothing.
        */
        void closeWindow(ui::Node& node);

        // ---- file_drop.cpp: перетаскивание файлов (только поток интерфейса) ----
        // ---- file_drop.cpp: dragging files (the UI thread only) ----

        /*
        ** Обработчик файлов, брошенных на элемент с onDrop="name" (из Проводника и других программ).
        ** Пока файлы несут над таким элементом, у него состояние <DropTarget> (подсветка в стиле);
        ** над остальными местами окна ОС показывает курсор «нельзя».
        **
        ** The handler of files dropped onto an element with onDrop="name" (from Explorer and other programs).
        ** While files are carried over such an element, it has the <DropTarget> state (a highlight in the style);
        ** over the other window places the OS shows the "no" cursor.
        */
        void onFileDrop(const std::string& name, FileDropHandler handler);

        // ---- application_files.cpp: системные диалоги файлов (только поток интерфейса) ----
        // ---- application_files.cpp: the system file dialogs (the UI thread only) ----

        /*
        ** Модальный диалог открытия над окном узла owner (nullptr — над главным): пути выбранных
        ** файлов в UTF-8 (multiple — можно выбрать несколько); пусто — пользователь отказался.
        ** Код после вызова продолжается, когда диалог закрыт. Ошибка ОС — GranException(PLATFORM).
        **
        ** A modal open dialog over the owner node window (nullptr — over the main one): the paths of the chosen
        ** files in UTF-8 (multiple — several can be chosen); empty — the user cancelled.
        ** The code after the call continues when the dialog is closed. An OS error is GranException(PLATFORM).
        */
        std::vector<std::string> openFileDialog(const FileDialogOptions& options, bool multiple = false,
            const ui::Node* owner = nullptr);

        // Диалог сохранения (с вопросом о замене файла); пусто — отказ / A save dialog (asking before replacing a file); empty — cancelled
        std::string saveFileDialog(const FileDialogOptions& options, const ui::Node* owner = nullptr);

        // Выбор папки; пусто — отказ / Choosing a folder; empty — cancelled
        std::string folderDialog(const FileDialogOptions& options, const ui::Node* owner = nullptr);

        // ---- application_frame.cpp ----

        // Узел по id во всех окнах, главное первым (nullptr — нет такого) / A node by id in all the windows, the main one first (nullptr — no such node)
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

        // ---- application_data.cpp: данные и разметка во время работы (только поток интерфейса) ----
        // ---- application_data.cpp: data and markup at run time (the UI thread only) ----

        /*
        ** Модель списка по имени (items="имя" в разметке); нет — создаётся пустая. Её можно
        ** заполнять до и после загрузки разметки: привязки строят и правят узлы сами. Данные
        ** <Data name="имя"> из разметки при загрузке заменяют содержимое модели.
        **
        ** A list model by name (items="name" in the markup); none — an empty one is created. It can be
        ** filled before and after loading the markup: the bindings build and edit the nodes themselves. The data
        ** <Data name="name"> from the markup replaces the model contents on loading.
        */
        data::ListModel& model(const std::string& name);

        /*
        ** Вставить фрагмент разметки последним ребёнком parent: один корневой элемент, проверка той же
        ** схемой, стили и тема — документа (ошибка — MarkupException с позицией во фрагменте).
        ** Списки items="…" во фрагменте привязываются сразу.
        **
        ** Insert a markup fragment as the last child of parent: one root element, checked by the same
        ** schema, the styles and the theme are the document ones (an error is MarkupException with a position in the fragment).
        ** The items="…" lists in the fragment are bound at once.
        */
        ui::Node& appendMarkup(ui::Node& parent, std::string_view markup);

        // Запись списка, из копии шаблона которой узел (ближайший список) — для обработчиков onClick в шаблоне
        // The list record whose template copy holds the node (the nearest list) — for onClick handlers in a template
        ItemRef itemOf(const ui::Node& node) const;

        // ---- application_web.cpp: встроенный браузер <WebView> (только поток интерфейса) ----
        // ---- application_web.cpp: the embedded <WebView> browser (the UI thread only) ----

        // Обработчик строк из страниц по имени действия onMessage / The handler of strings from pages by the onMessage action name
        void onWebMessage(const std::string& name, WebMessageHandler handler);

        /*
        ** Строка в страницу (событие message у window.chrome.webview). До создания браузера
        ** и загрузки страницы сообщения копятся и уходят по порядку. Не WebView — GranException.
        **
        ** A string into the page (the message event of window.chrome.webview). Before the browser is created
        ** and the page is loaded the messages accumulate and go out in order. Not a WebView — GranException.
        */
        void postWebMessage(ui::Node& webView, const std::string& message);

        // Открыть адрес (как атрибут source) / Open an address (like the source attribute)
        void navigate(ui::Node& webView, const std::string& url);

        // Показать HTML (как текст тега; адрес source сбрасывается) / Show HTML (like the tag text; the source address is reset)
        void loadHtml(ui::Node& webView, const std::string& html);

        // ---- application_popups.cpp: всплывающие слои (<Popup>), только во время run() / popup layers (<Popup>), only during run() ----

        // Показать слой под узлом anchor (над ним, если внизу нет места) — выпадающий список / Show the layer under the anchor node (over it if there is no room below) — a drop-down list
        void showPopup(ui::Node& popup, const ui::Node& anchor);

        // Показать слой в точке окна (логические пиксели) — контекстное меню / Show the layer at a window point (logical pixels) — a context menu
        void showPopupAt(ui::Node& popup, Vec2 point);

        // Скрыть слой (Esc, щелчок мимо и выбор пункта меню скрывают его сами) / Hide the layer (Esc, a click outside and choosing a menu item hide it themselves)
        void hidePopup(ui::Node& popup);

        // ---- application_dialogs.cpp: диалоги и выпадающие списки / dialogs and drop-down lists ----

        /*
        ** Показать модальный диалог <Dialog> по центру окна: окно под ним затемняется, ввод вне
        ** диалога блокируется, фокус — на первый элемент внутри. Не Dialog — GranException; до run() — ничего.
        **
        ** Show a modal <Dialog> in the window center: the window under it is dimmed, the input outside
        ** the dialog is blocked, the focus goes to the first control inside. Not a Dialog — GranException; before run() — nothing.
        */
        void showDialog(ui::Node& dialog);

        // Закрыть диалог; фокус возвращается туда, где был до открытия / Close the dialog; the focus returns where it was before opening
        void hideDialog(ui::Node& dialog);

        /*
        ** Выбрать в <ComboBox> пункт со значением value из кода (как value в разметке; onChange
        ** не вызывается); пункта нет — выбор снимается, видна подсказка. Значение — comboBox.content.option.
        ** Не ComboBox — GranException.
        **
        ** Select the item with the value value in a <ComboBox> from code (as value in the markup; onChange
        ** is not called); no such item — the choice is cleared, the placeholder is shown. The value is comboBox.content.option.
        ** Not a ComboBox — GranException.
        */
        void selectOption(ui::Node& comboBox, const std::string& value);

        /*
        ** Выбрать в <Tabs> страницу со значением value из кода (onChange не вызывается); страницы нет —
        ** выбор не меняется. Значение — tabs.content.option. Не Tabs — GranException.
        **
        ** Select the page with the value value in <Tabs> from code (onChange is not called); no such page —
        ** the choice does not change. The value is tabs.content.option. Not Tabs — GranException.
        */
        void selectTab(ui::Node& tabs, const std::string& value);

        // ---- application_values.cpp: значения элементов из кода (onChange не вызывается) ----
        // ---- application_values.cpp: control values from code (onChange is not called) ----

        // Число ProgressBar или Slider, ограниченное min и max; другой вид — GranException
        // The number of a ProgressBar or a Slider clamped by min and max; another kind — GranException
        void setValue(ui::Node& node, float value);

        /*
        ** Отметка CheckBox, Switch или RadioButton (отметка радиокнопки снимает её с остальных в группе);
        ** другой вид — GranException.
        **
        ** The mark of a CheckBox, a Switch or a RadioButton (checking a radio button unchecks the others in the group);
        ** another kind — GranException.
        */
        void setChecked(ui::Node& node, bool checked);

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

        // ---- application_timers.cpp: таймеры (только поток интерфейса; из других потоков — через post) ----
        // ---- application_timers.cpp: timers (the UI thread only; from other threads — through post) ----

        /*
        ** Однократный таймер: handler выполнится в потоке интерфейса не раньше чем через seconds
        ** секунд (точность — кадр или пробуждение цикла), после него кадр перерисуется. Таймеры,
        ** поставленные до run(), отсчитываются с момента постановки и срабатывают внутри run().
        **
        ** A one-shot timer: handler runs in the UI thread not earlier than seconds
        ** seconds later (the precision is a frame or a loop wake-up), the frame is redrawn after it. Timers
        ** set before run() count from the moment of setting and fire inside run().
        */
        TimerId setTimeout(double seconds, Task handler);

        // Периодический таймер (период не меньше 1 мс); пропущенные срабатывания не догоняются / A periodic timer (the period is at least 1 ms); missed firings are not caught up
        TimerId setInterval(double seconds, Task handler);

        // Снять таймер (можно из его же обработчика); неизвестный номер — ничего / Clear a timer (also from its own handler); an unknown number — nothing
        void clearTimer(TimerId id);

        // ---- application_run.cpp ----

        // Окно и цикл событий до закрытия; возвращает код выхода / The window and the event loop until closing; returns the exit code
        int run();

        // Управление окном в UI-потоке; до run окно ещё не создано. close завершает цикл.
        // Window management on the UI thread; before run no window exists yet. close ends the loop.
        void minimize();
        void maximize();
        void restore();
        void close();
        bool isMaximized() const;

        struct State;

    private:
        std::unique_ptr<State> _state;
    };
}

#endif
