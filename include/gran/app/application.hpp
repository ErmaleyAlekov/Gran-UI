#ifndef GRAN_APP_APPLICATION_HPP
#define GRAN_APP_APPLICATION_HPP

#include "gran/binding/list_bindings.hpp"
#include "gran/ui/node.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace gran
{
    // Обработчик действия разметки (onClick="имя") / A markup action handler (onClick="name")
    using ActionHandler = std::function<void(ui::Node& node)>;

    // Обработчик строки из страницы <WebView onMessage="имя"> / A handler of a string from a <WebView onMessage="name"> page
    using WebMessageHandler = std::function<void(ui::Node& webView, const std::string& message)>;

    // Задача для потока интерфейса (Application::post) / A task for the UI thread (Application::post)
    using Task = std::function<void()>;

    // Запись списка, к которой относится узел (Application::itemOf); model == nullptr — узел не из списка
    // The list record a node belongs to (Application::itemOf); model == nullptr — the node is not from a list
    using ItemRef = binding::ItemRef;

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

        // ---- application_run.cpp ----

        // Окно и цикл событий до закрытия; возвращает код выхода / The window and the event loop until closing; returns the exit code
        int run();

        struct State;

    private:
        std::unique_ptr<State> _state;
    };
}

#endif
