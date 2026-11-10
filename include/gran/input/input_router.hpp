#ifndef GRAN_INPUT_INPUT_ROUTER_HPP
#define GRAN_INPUT_INPUT_ROUTER_HPP

#include "gran/a11y/access_tree.hpp"
#include "gran/platform/window.hpp"
#include "gran/ui/command.hpp"
#include "gran/ui/node.hpp"
#include "gran/ui/ui_event.hpp"

#include <string>
#include <vector>

namespace gran::input
{
    /*
    ** Нажатое сочетание и где оно нажато: узел фокуса (nullptr — нет) и открытый модальный диалог
    ** (nullptr — нет; при нём действуют только команды области focus внутри диалога).
    **
    ** The pressed combination and where it was pressed: the focus node (nullptr — none) and the open modal dialog
    ** (nullptr — none; with it only the focus scope commands inside the dialog work).
    */
    struct CommandQuery
    {
        ui::Shortcut pressed;
        ui::Node* focus = nullptr;
        const ui::Node* modal = nullptr;
    };

    /*
    ** Команды одного окна для поиска по сочетанию: root — корень окна (для within), own — окно, где нажали
    ** (его команды window и focus действуют, у прочих окон — только global).
    **
    ** The commands of one window for the search by a combination: root is the window root (for within), own — the window
    ** where it was pressed (its window and focus commands work, for the other windows only global ones).
    */
    struct CommandSource
    {
        std::vector<ui::Command>* commands = nullptr;
        ui::Node* root = nullptr;
        bool own = false;
    };

    /*
    ** Получатель действий приложения: onClick="имя", onChange, onSubmit. Приложение
    ** сопоставляет имя со своей функцией (паттерн Command: разметка хранит имя команды).
    **
    ** The application action receiver: onClick="name", onChange, onSubmit. The application
    ** maps the name to its function (the Command pattern: the markup keeps the command name).
    */
    class ActionSink
    {
    public:
        virtual ~ActionSink() = default;

        // Действие с именем name от узла node / The action named name from the node
        virtual void action(const std::string& name, ui::Node& node) = 0;

        // ---- action_sink.cpp ----

        /*
        ** Обработчик события из разметки (onPointerDown="name"): node — узел с атрибутом (current).
        ** По умолчанию — то же, что действие action(name, node): onAction и встроенные gran:… работают и здесь.
        **
        ** An event handler from the markup (onPointerDown="name"): node is the node with the attribute (current).
        ** By default it is the same as the action(name, node): onAction and the built-in gran:… work here too.
        */
        virtual void event(const std::string& name, ui::Node& node, ui::UiEvent& event);

        /*
        ** Счётчик удалений узлов: растёт, когда из дерева что-то удалено. Изменился во время обработчика —
        ** указатели на узлы могли повиснуть, доставка и действие по умолчанию прекращаются. По умолчанию 0.
        **
        ** The node removal counter: it grows when something is removed from the tree. It changed during a handler —
        ** the node pointers might dangle, the delivery and the default action stop. By default 0.
        */
        virtual std::uint32_t treeVersion() const noexcept;

        /*
        ** Горячая клавиша: найти включённую команду по сочетанию и выполнить; true — выполнена (клавиша
        ** поглощена). По умолчанию команд нет — false.
        **
        ** A shortcut: find an enabled command by the combination and run it; true — it ran (the key
        ** is consumed). By default there are no commands — false.
        */
        virtual bool shortcut(const CommandQuery& query);

        // Кнопка или пункт меню с command="id" активирован (node — он сам); по умолчанию — ничего
        // A button or a menu item with command="id" is activated (node is itself); by default — nothing
        virtual void command(const std::string& id, ui::Node& node);

        /*
        ** Сменилось редактируемое поле (фокус) или его текст и история: команды отмены пересчитывают
        ** включённость (gran:undo работает и с полем). По умолчанию — ничего.
        **
        ** The edited field (the focus) or its text and history changed: the undo commands recompute
        ** their enabled state (gran:undo works with the field too). By default — nothing.
        */
        virtual void editingChanged();

        // ---- action_sink_lists.cpp ----

        /*
        ** Нажатие в контейнере списка с моделью выделения (selection="…"): hit — узел под мышью,
        ** modifiers — platform::Modifier. По умолчанию — ничего.
        **
        ** A press in a list container with a selection model (selection="…"): hit is the node under the mouse,
        ** modifiers are platform::Modifier. By default — nothing.
        */
        virtual void listPress(ui::Node& container, ui::Node& hit, std::uint8_t modifiers);

        // Клавиша у такого контейнера в фокусе; true — обработана (стрелки, Home/End, пробел, Ctrl+A) / A key at such a focused container; true — handled (the arrows, Home/End, space, Ctrl+A)
        virtual bool listKey(ui::Node& container, platform::Key key, std::uint8_t modifiers);
    };

    /*
    ** Поиск места в тексте узла по точке. Раскладку текста знает только рендерер, поэтому
    ** маршрутизатор получает её через этот интерфейс (паттерн Strategy) и не зависит от render.
    **
    ** Finding a place in the node text by a point. Only the renderer knows the text layout, so
    ** the router gets it through this interface (the Strategy pattern) and does not depend on render.
    */
    class TextLocator
    {
    public:
        virtual ~TextLocator() = default;

        // Байт UTF-8 места каретки, ближайшего к точке окна point / The UTF-8 byte of the caret place nearest to the window point
        virtual std::size_t offsetAt(const ui::Node& node, Vec2 point) = 0;

        // Байт начала (end = false) или конца строки, на которой стоит байт offset / The byte of the start (end = false) or the end of the line holding the byte offset
        virtual std::size_t lineEdge(const ui::Node& node, std::size_t offset, bool end) = 0;

        // Байт на lines строк выше (< 0) или ниже; goalX — желаемый x (NaN — x байта offset), на выходе — использованный
        // The byte lines lines above (< 0) or below; goalX is the wanted x (NaN — the x of the byte offset), on output — the one used
        virtual std::size_t lineMove(const ui::Node& node, std::size_t offset, int lines, float& goalX) = 0;

        // Строк в видимой части поля (шаг PageUp/PageDown) / The lines in the visible part of the field (the PageUp/PageDown step)
        virtual int pageLines(const ui::Node& node) = 0;
    };

    /*
    ** Открытый слой и узел, куда вернётся фокус при его закрытии (фокус до открытия). У каждого слоя
    ** свой узел возврата: подменю возвращает фокус пункту родительского меню, меню — кнопке, диалог — тому,
    ** что было в фокусе до него.
    **
    ** An open layer and the node the focus returns to on its closing (the focus before opening). Every layer
    ** has its own return node: a submenu returns the focus to the parent menu item, a menu — to the button, a dialog —
    ** to what was focused before it.
    */
    struct OpenLayer
    {
        ui::Node* node = nullptr;
        ui::Node* returnFocus = nullptr;
    };

    /*
    ** Внутреннее перетаскивание узла: source — нажатый перетаскиваемый узел (кандидат, пока мышь не ушла
    ** дальше порога), origin — точка нажатия, active — перетаскивание идёт (dragStart не отменён), data — формат
    ** и данные, zone — зона под мышью, принимающая формат, allowed — зона согласна (dragOver не отменён).
    **
    ** The internal node drag: source is the pressed draggable node (a candidate until the mouse goes past
    ** the threshold), origin is the press point, active — the drag goes on (dragStart is not cancelled), data is the format
    ** and the data, zone is the zone under the mouse that accepts the format, allowed — the zone agrees (dragOver is not cancelled).
    */
    struct NodeDrag
    {
        ui::Node* source = nullptr;
        Vec2 origin;
        bool active = false;
        ui::DragData data;
        ui::Node* zone = nullptr;
        bool allowed = false;
    };

    /*
    ** Состояние маршрутизатора: корень, получатели и узлы под мышью, нажатый, в фокусе, перетаскиваемый
    ** ползунок; selection — узел с текущим выделением (поле ввода или Text), selecting — узел,
    ** в котором мышь сейчас тянет выделение.
    **
    ** The router state: the root, the receivers and the nodes under the mouse, pressed, focused, the dragged
    ** slider; selection is the node with the current selection (an input or a Text), selecting is the node
    ** where the mouse is dragging the selection now.
    **
    ** goalNode, goalCaret, goalX — желаемый x каретки при движении по строкам подряд (стрелки вверх
    ** и вниз): действует, пока каретка стоит там, куда её поставил прошлый шаг.
    ** goalNode, goalCaret, goalX are the wanted caret x while moving by lines in a row (the up and down
    ** arrows): it holds while the caret stands where the previous step put it.
    */
    struct RouterState
    {
        ui::Node* root = nullptr;
        ActionSink* actions = nullptr;
        platform::Window* window = nullptr;
        TextLocator* text = nullptr;
        ui::Node* hover = nullptr;
        ui::Node* pressed = nullptr;
        ui::Node* focus = nullptr;
        ui::Node* drag = nullptr;
        ui::Node* selection = nullptr;
        ui::Node* selecting = nullptr;
        const ui::Node* goalNode = nullptr;
        std::size_t goalCaret = 0;
        float goalX = 0.0f;
        // Открытые слои снизу вверх; встроенное контекстное меню и его цель; последняя точка мыши
        // The open layers bottom to top; the built-in context menu and its target; the last mouse point
        std::vector<OpenLayer> layers;
        ui::Node* contextMenu = nullptr;
        ui::Node* contextTarget = nullptr;
        Vec2 pointer;
        // Текущее событие — нажатие клавиши: открытое им меню сразу ставит фокус на пункт (как в Windows)
        // The current event is a key press: a menu opened by it puts the focus on an item at once (as in Windows)
        bool keyboard = false;
        // Захват указателя: события мыши идут этому узлу, где бы ни была мышь (до отпускания кнопки)
        // The pointer capture: the mouse events go to this node wherever the mouse is (until the button release)
        ui::Node* capture = nullptr;
        NodeDrag nodeDrag;
        // Отложенные уведомления (focus, blur, pointerEnter, pointerLeave): доставляются после встроенной обработки события
        // The deferred notifications (focus, blur, pointerEnter, pointerLeave): delivered after the built-in event handling
        std::vector<ui::UiEvent> pending;
        // Щелчков у последнего нажатия левой кнопки (2 — после отпускания будет doubleClick)
        // The clicks of the last left button press (2 — doubleClick comes after the release)
        std::uint8_t downClicks = 0;
        // keyDown отменён приложением: символы этой клавиши в поле не вставляются / keyDown is cancelled by the application: the characters of this key are not inserted into a field
        bool suppressCharacter = false;
        // Обработчиков вызвано за текущее событие окна: были — нужен кадр (обработчик мог что-то поменять)
        // The handlers called during the current window event: there were some — a frame is needed (a handler might change something)
        std::uint32_t delivered = 0;
        // Строка меню (menu_bar_*.cpp): Alt нажат один (его отпускание войдёт в строку), Alt зажат, меню открыты
        // с клавиатуры, видны ли сейчас подчёркивания букв доступа и узел фокуса до входа в строку меню
        // The menu bar (menu_bar_*.cpp): Alt is pressed alone (its release enters the bar), Alt is held, the menus are open
        // from the keyboard, whether the access letter underlines are shown now and the focus node before entering the bar
        bool altArmed = false;
        bool altHeld = false;
        bool keyboardMenus = false;
        bool cuesShown = false;
        ui::Node* barReturn = nullptr;
        // Место захвата ручки разделителя по главной оси от её начала (splitter_events.cpp)
        // The place the splitter handle was grabbed at, along the main axis from its start (splitter_events.cpp)
        float splitGrab = 0.0f;
        // Перетаскиваемый заголовок вкладки уже сдвигался (по отпусканию — onReorder) / The dragged tab header has already moved (on the release — onReorder)
        bool tabMoved = false;
    };

    /*
    ** Маршрутизатор ввода: события окна → состояния узлов (hover, pressed, focused, checked),
    ** действия приложения, правка текста, прокрутка. Узлы, которым нужно пересчитать стиль
    ** или компоновку, получают флаги «грязно» — кадр рисуется, только если что-то изменилось.
    **
    ** The input router: window events → node states (hover, pressed, focused, checked),
    ** application actions, text editing, scrolling. The nodes that need a style or layout recompute
    ** get the "dirty" flags — a frame is drawn only if something changed.
    */
    class InputRouter
    {
    public:
        // ---- input_router.cpp ----
        InputRouter(ui::Node& root, ActionSink& actions, platform::Window* window, TextLocator& text);

        // Событие окна; true — изменилось что-то видимое / A window event; true — something visible changed
        bool handle(const platform::Event& event);

        // Курсор над узлом под мышью / The cursor over the node under the mouse
        platform::CursorShape cursor() const noexcept;

        // Узел в фокусе (для каретки поля ввода) / The focused node (for the input caret)
        const ui::Node* focused() const noexcept;

        // Узел, выделение которого видно (Text или поле ввода в фокусе), или nullptr
        // The node whose selection is visible (a Text or the focused input), or nullptr
        const ui::Node* selected() const noexcept;

        // Видны ли подчёркивания букв доступа меню (Alt зажат или меню открыты с клавиатуры) / Whether the menu access letter underlines are shown (Alt held or the menus are open from the keyboard)
        bool mnemonicsShown() const noexcept;

        // ---- input_router_tree.cpp ----

        // Фокус из кода (nullptr — снять); поле ввода получает каретку без выделения / The focus from code (nullptr — remove); an input gets the caret without a selection
        void focus(ui::Node* node);

        // Поддерево удаляется из дерева: ссылки на его узлы обнуляются / The subtree is removed from the tree: the references to its nodes are cleared
        void forget(const ui::Node& subtree) noexcept;

        // ---- input_router_popups.cpp ----

        // Открыть слой у рамки якоря (окно); passive — подсказка: не ловит мышь, не забирает фокус
        // Opening a layer at the anchor frame (window); passive — a tooltip: does not catch the mouse, does not take the focus
        void openPopup(ui::Node& popup, const Rect& anchor, bool passive);

        // Закрыть слой (фокус из него возвращается туда, где был до открытия) / Closing a layer (the focus in it returns where it was before opening)
        void closePopup(ui::Node& popup);

        // Узел под мышью / The node under the mouse
        const ui::Node* hovered() const noexcept;

        // Последняя точка мыши в окне / The last mouse point in the window
        Vec2 pointer() const noexcept;

        // Роль окна с тем же отсечением, преобразованиями и модальными слоями, что у мыши.
        // The window role with the same clipping, transforms and modal layers as pointer input.
        platform::WindowRole windowRoleAt(Vec2 point);

        // ---- access_actions.cpp ----

        // Запрос клиента доступности (нажать, переключить, текст, число, фокус); true — что-то изменилось
        // An accessibility client request (press, toggle, text, number, focus); true — something changed
        bool perform(const a11y::AccessRequest& request);

        // ---- input_router_events.cpp ----

        // Своё событие из кода: доставка по пути цели и отложенные уведомления; true — действие не отменено
        // A custom event from code: the delivery along the target path and the deferred notifications; true — the action is not cancelled
        bool dispatch(ui::UiEvent& event);

        // Захватить указатель узлом (nullptr — отпустить); захват снимается и отпусканием кнопки мыши
        // Capture the pointer by the node (nullptr — release it); the capture is also removed by the mouse button release
        void capturePointer(ui::Node* node) noexcept;

        // Узел, захвативший указатель, или nullptr / The node that captured the pointer or nullptr
        const ui::Node* pointerCapture() const noexcept;

        // Идёт ли перетаскивание узла / Whether a node drag goes on
        bool draggingNode() const noexcept;

        // ---- field_history.cpp ----

        // Есть ли в редактируемом поле что отменить (redo — повторить) / Whether the edited field has something to undo (redo — to redo)
        bool canUndoField(bool redo);

        // Отменить / повторить правку редактируемого поля; false — нечего (тогда — стек приложения) / Undo / redo an edit of the edited field; false — nothing (then — the application stack)
        bool undoField(bool redo);

    private:
        RouterState _state;
    };

    // ---- event_dispatch.cpp ----

    /*
    ** Доставка события по пути цели: захват (корень → родитель цели), цель, всплытие (если bubbles) —
    ** обработчики разметки узла, затем слушатели кода. Удаление узлов обработчиком (treeVersion) прекращает
    ** доставку. true — действие по умолчанию не отменено.
    **
    ** Delivering the event along the target path: capture (the root → the target parent), the target, bubbling (if bubbles) —
    ** the node markup handlers, then the code listeners. Removing nodes by a handler (treeVersion) stops
    ** the delivery. true — the default action is not cancelled.
    */
    bool dispatchEvent(ui::UiEvent& event, ActionSink& sink);

    // ---- event_queue.cpp ----

    // Доставить сейчас; true — действие по умолчанию разрешено (не отменено, дерево не менялось) / Deliver now; true — the default action is allowed (not cancelled, the tree did not change)
    bool fire(RouterState& state, ui::UiEvent& event);

    // Отложить уведомление до конца обработки события окна / Defer a notification until the end of the window event handling
    void queueEvent(RouterState& state, ui::UiEvent event);

    // Доставить отложенные уведомления по порядку / Deliver the deferred notifications in order
    void flushEvents(RouterState& state);

    // Отложенные уведомления об узлах поддерева отбрасываются (поддерево удаляется) / The deferred notifications about the subtree nodes are dropped (the subtree is removed)
    void forgetEvents(RouterState& state, const ui::Node& subtree) noexcept;

    // ---- pointer_events.cpp ----

    // Событие указателя вида type для узла target из события окна / A pointer event of the type kind for the target node from a window event
    ui::UiEvent pointerEvent(ui::EventType type, ui::Node& target, const platform::Event& source);

    // Цель событий указателя: узел захвата, иначе узел под мышью / The pointer event target: the capture node, otherwise the node under the mouse
    ui::Node* pointerTarget(const RouterState& state, ui::Node* hit) noexcept;

    // Событие указателя сейчас; target == nullptr — нечего доставлять (true) / A pointer event now; target == nullptr — nothing to deliver (true)
    bool firePointer(RouterState& state, ui::EventType type, ui::Node* target, const platform::Event& source);

    // ---- notify_events.cpp ----

    // pointerLeave узлам старой цепочки и pointerEnter новой (отложенно) / pointerLeave to the old chain nodes and pointerEnter to the new one (deferred)
    void notifyHover(RouterState& state, ui::Node* from, ui::Node* to);

    // blur старому узлу фокуса и focus новому (отложенно) / blur to the old focus node and focus to the new one (deferred)
    void notifyFocus(RouterState& state, ui::Node* from, ui::Node* to);

    // ---- key_events.cpp ----

    // keyDown или keyUp узлу фокуса (без него — верхнему слою или корню); true — действие разрешено
    // keyDown or keyUp to the focus node (without it — to the top layer or the root); true — the action is allowed
    bool fireKey(RouterState& state, const platform::Event& source);

    // Клавиша меню и Shift+F10: contextMenu узлу фокуса, встроенное меню у текста; true — обработано
    // The menu key and Shift+F10: contextMenu to the focus node, the built-in menu at a text; true — handled
    bool contextKey(RouterState& state, const platform::Event& source);

    // Esc во время перетаскивания узла отменяет его; true — обработано / Esc during a node drag cancels it; true — handled
    bool dragKey(RouterState& state, const platform::Event& source);

    // ---- press_input.cpp ----

    // Нажатие левой кнопки: выделение текста, нажатый элемент, фокус, ползунок, кандидат перетаскивания
    // A left button press: the text selection, the pressed control, the focus, a slider, a drag candidate
    void pressLeft(RouterState& state, ui::Node* hit, const platform::Event& event);

    // Отпускание левой кнопки: активация элемента, над которым и нажали (кроме ползунка), затем doubleClick
    // A left button release: activating the control it was pressed over (except a slider), then doubleClick
    void releaseLeft(RouterState& state, ui::Node* hit, const platform::Event& event, bool allowed);

    // Запомнить кандидата перетаскивания и его данные по умолчанию (nullptr — нет) / Remember the drag candidate and its default data (nullptr — none)
    void armDrag(RouterState& state, ui::Node* draggable, Vec2 point);

    // ---- node_drag.cpp ----

    // Движение мыши: начало за порогом (dragStart), зона и dragOver; true — событие поглощено перетаскиванием
    // A mouse move: the start past the threshold (dragStart), the zone and dragOver; true — the event is consumed by the drag
    bool moveDrag(RouterState& state, const platform::Event& event);

    // Отпускание: drop в согласную зону и dragEnd источнику; true — шло перетаскивание
    // A release: drop into an agreeing zone and dragEnd to the source; true — a drag was going on
    bool finishDrag(RouterState& state, const platform::Event& event);

    // Отмена (Esc, потеря фокуса окном): dragLeave зоне и dragEnd источнику / Cancelling (Esc, the window losing focus): dragLeave to the zone and dragEnd to the source
    void cancelDrag(RouterState& state);

    // ---- drag_zone.cpp ----

    // Перетаскиваемый узел под нажатием: сам hit или предок с draggable, если элемент управления не глубже его
    // The draggable node under a press: hit itself or an ancestor with draggable if the control is not deeper than it
    ui::Node* draggableOf(ui::Node* hit, ui::Node* control) noexcept;

    // Ближайшая к точке включённая зона, принимающая формат перетаскивания (вне поддерева источника)
    // The enabled zone nearest to the point that accepts the drag format (outside the source subtree)
    ui::Node* dropZoneAt(RouterState& state, Vec2 point);

    // Смена зоны: состояние DropTarget, dragLeave старой и dragEnter новой (notify) / Changing the zone: the DropTarget state, dragLeave to the old one and dragEnter to the new one (notify)
    void changeZone(RouterState& state, ui::Node* zone, const platform::Event& source, bool notify);

    // Событие перетаскивания для узла node: данные, related — второй участник / A drag event for the node: the data, related is the second participant
    ui::UiEvent dragEvent(const RouterState& state, ui::EventType type, ui::Node& node, const platform::Event& source);

    // ---- hit_test.cpp ----

    // Самый глубокий видимый узел в точке (с учётом отсечения) / The deepest visible node at the point (respecting clipping)
    ui::Node* hitTest(ui::Node& root, Vec2 point);

    // Самый глубокий узел слоя Popup в точке окна / The deepest node of a Popup layer at the window point
    ui::Node* hitLayer(ui::Node& popup, Vec2 point);

    // ---- node_states.cpp ----

    // Бит состояния одному узлу (изменение — пересчёт стиля) / A state bit to one node (a change — a style recompute)
    void setNodeState(ui::Node& node, std::uint16_t bit, bool on);

    // Бит состояния узлу и всем предкам (hover и pressed в CSS действуют на цепочку) / A state bit to the node and all ancestors
    void setChainState(ui::Node* node, std::uint16_t bit, bool on);

    // Смена узла под мышью, нажатого, в фокусе / Changing the hovered, pressed, focused node
    void changeHover(RouterState& state, ui::Node* node);
    void changePressed(RouterState& state, ui::Node* node);
    void changeFocus(RouterState& state, ui::Node* node);

    // ---- interaction.cpp ----

    // Ближайший предок-элемент управления (кнопка, флажок, ползунок, поле, onClick); nullptr — нет
    // The nearest control ancestor (a button, a check box, a slider, an input, onClick); nullptr — none
    ui::Node* controlOf(ui::Node* node) noexcept;

    // Выполнить роль кнопки окна до пользовательского onClick.
    // Perform the window button role before the user onClick.
    void activateWindow(RouterState& state, const ui::Node& node);

    // Узел или предок отключён / The node or an ancestor is disabled
    bool disabled(const ui::Node* node) noexcept;

    // Ближайший предок-прокрутка / The nearest scroll ancestor
    ui::Node* scrollOf(ui::Node* node) noexcept;

    // Следующий (или предыдущий) элемент, принимающий фокус, в порядке дерева / The next (or previous) focusable element in tree order
    ui::Node* nextFocusable(ui::Node& root, const ui::Node* current, bool backward);

    // Активация элемента: клик кнопки, переключение флажка (с действиями) / Activating a control: a button click, a check box toggle (with actions)
    void activate(RouterState& state, ui::Node& control);

    // ---- choice_input.cpp ----

    // Выбор у флажка и переключателя (смена отметки), радиокнопки и заголовка вкладки; прочие виды — ничего
    // The choice of a check box and a switch (toggling the mark), a radio button and a tab header; other kinds — nothing
    void changeChoice(RouterState& state, ui::Node& control);

    // Стрелка у члена группы (радиокнопки, заголовка вкладки): фокус и выбор — соседу; false — не обработано
    // An arrow on a group member (a radio button, a tab header): the focus and the choice go to a neighbor; false — not handled
    bool stepChoice(RouterState& state, ui::Node& member, platform::Key key);

    // ---- pointer_input.cpp ----

    // События мыши; true — что-то изменилось / Mouse events; true — something changed
    bool handlePointer(RouterState& state, const platform::Event& event);

    // ---- slider_input.cpp ----

    // Значение ползунка с шагом и в пределах диапазона; изменилось — перерисовка и onChange; true — изменилось
    // The slider value with the step and within the range; on a change — a repaint and onChange; true — changed
    bool setSliderValue(RouterState& state, ui::Node& slider, float value);

    // Значение ползунка по координате мыши (с шагом) и действие onChange / The slider value by the mouse coordinate (with the step) and the onChange action
    void dragSlider(RouterState& state, ui::Node& slider, float x);

    // Клавиши стрелок у ползунка; true — значение изменилось / The arrow keys on a slider; true — the value changed
    bool stepSlider(RouterState& state, ui::Node& slider, int direction);

    // ---- span_links.cpp ----

    // Участок Span с onClick под точкой окна внутри Text hit; иначе — сам hit / The Span run with onClick under the window point inside the Text hit; otherwise — hit itself
    ui::Node* spanAt(RouterState& state, ui::Node* hit, Vec2 point);

    // ---- popup_layer.cpp ----

    // Самый верхний узел в точке: открытые слои (кроме пассивных) сверху вниз, затем дерево / The topmost node at the point: the open layers (except passive ones) top down, then the tree
    ui::Node* pickNode(RouterState& state, Vec2 point);

    // Открыть слой; повторное открытие поднимает его наверх / Opening a layer; opening again raises it to the top
    void openLayer(RouterState& state, ui::Node& popup, const Rect& anchor, bool passive);

    // Закрыть слой и вернуть фокус / Closing a layer and returning the focus
    void closeLayer(RouterState& state, ui::Node& popup);

    // Нажатие мимо всех непассивных слоёв закрывает их; true — нажатие поглощено / A press outside all non-passive layers closes them; true — the press is consumed
    bool closeOutside(RouterState& state, Vec2 point);

    // Выбор пункта меню: цепочка меню закрывается, затем значение ComboBox и команда (встроенная или onClick)
    // Choosing a menu item: the menu chain closes, then the ComboBox value and the command (built-in or onClick)
    void chooseItem(RouterState& state, ui::Node& item);

    // ---- layer_stack.cpp ----

    // Лежит ли узел в поддереве subtree (подъём по родителям) / Whether the node lies in the subtree (climbing the parents)
    bool within(const ui::Node* node, const ui::Node& subtree) noexcept;

    // Верхний непассивный слой (подсказки клавиш и мыши не ловят) или nullptr / The top non-passive layer (tooltips catch neither keys nor the mouse) or nullptr
    ui::Node* topLayer(const RouterState& state) noexcept;

    // ---- field_history.cpp ----

    /*
    ** Редактируемое поле: поле ввода в фокусе, а если фокус в открытом меню — поле, откуда меню открыли
    ** (фокус вернётся туда при закрытии). Диалог — свой мир фокуса: дальше него не ищется. nullptr — нет.
    **
    ** The edited field: the focused input, and if the focus is in an open menu — the field the menu was opened from
    ** (the focus returns there on closing). A dialog is its own focus world: the search does not go past it. nullptr — none.
    */
    ui::Node* editingField(const RouterState& state) noexcept;

    // Верхний открытый диалог: ввод ниже него заблокирован; nullptr — модального слоя нет
    // The top open dialog: the input below it is blocked; nullptr — there is no modal layer
    ui::Node* modalLayer(const RouterState& state) noexcept;

    // Закрыть открытые слои внутри поддерева layer (подменю, списки, меню диалога), сам layer — нет
    // Closing the open layers inside the layer subtree (submenus, lists, dialog menus), not the layer itself
    void closeNested(RouterState& state, const ui::Node& layer);

    // Закрыть меню над верхним диалогом (всю цепочку подменю) / Closing the menus over the top dialog (the whole submenu chain)
    void closeMenus(RouterState& state);

    // ---- menu_open.cpp ----

    // Открыть слой, привязанный к элементу (Button popup="id", список ComboBox); false — привязанного слоя нет
    // Opening the layer attached to a control (Button popup="id", a ComboBox list); false — there is no attached layer
    bool openAttached(RouterState& state, ui::Node& control);

    // Открыть подменю пункта сбоку от него / Opening the item submenu at its side
    void openSubmenu(RouterState& state, ui::Node& item);

    // Наведение на пункт: подменю соседних пунктов закрываются, своё — открывается / Hovering an item: the neighbor item submenus close, its own one opens
    void hoverMenus(RouterState& state, ui::Node* hover);

    // Фокус при открытии: диалог — первый элемент внутри, список ComboBox — выбранный пункт, меню с клавиатуры — первый пункт
    // The focus on opening: a dialog — the first control inside, a ComboBox list — the selected item, a menu from the keyboard — the first item
    void focusOnOpen(RouterState& state, ui::Node& layer);

    // ---- menu_bar_input.cpp ----

    // Состояние Open у пункта меню или ComboBox, чей слой открыт или закрыт / The Open state on a menu item or a ComboBox whose layer opens or closes
    void markOpen(ui::Node& layer, bool open);

    // Войти в строку меню на пункт item (прежний фокус запоминается); open — сразу открыть его меню (или выполнить пункт без меню)
    // Entering the menu bar at item (the former focus is remembered); open — open its menu at once (or run an item without a menu)
    void enterMenuBar(RouterState& state, ui::Node& item, bool open);

    // Выйти из строки меню: её меню закрываются, фокус — туда, где был до входа / Leaving the menu bar: its menus close, the focus goes where it was before entering
    void leaveMenuBar(RouterState& state);

    // Alt (нажать и отпустить без других клавиш) и F10 — войти в строку меню или выйти; true — клавиша обработана
    // Alt (press and release without other keys) and F10 — enter the menu bar or leave it; true — the key is handled
    bool altKey(RouterState& state, const platform::Event& event);

    // Учёт событий для Alt и подсказок до обработки (мышь, потеря фокуса) и после неё (видимость подчёркиваний); true — нужен кадр
    // Accounting events for Alt and the cues before the handling (the mouse, the focus loss) and after it (the underline visibility); true — a frame is needed
    bool trackMenuCues(RouterState& state, const platform::Event& event, bool after);

    // ---- menu_bar_keys.cpp ----

    // Клавиши на пункте строки меню без открытого меню: стрелки вбок — сосед, вниз, Enter, пробел — открыть, Esc — выйти
    // The keys on a menu bar item without an open menu: the side arrows — a neighbor, down, Enter, space — open, Esc — leave
    bool menuBarKey(RouterState& state, const platform::Event& event);

    // Стрелки вбок в меню пункта строки: соседний пункт строки и его меню; false — не тот случай
    // The side arrows in the menu of a bar item: the neighbor bar item and its menu; false — not that case
    bool barSideKey(RouterState& state, ui::Node& menu, platform::Key key);

    // Наведение на другой пункт строки, пока открыто меню строки, переключает меню / Hovering another bar item while a bar menu is open switches the menu
    void hoverMenuBar(RouterState& state, ui::Node* hover);

    // Символ как буква доступа: пункт открытого меню, пункт строки меню (в строке или с Alt); true — символ поглощён
    // A character as an access letter: an item of the open menu, a menu bar item (in the bar or with Alt); true — the character is consumed
    bool mnemonicKey(RouterState& state, const platform::Event& event);

    // Открытое меню пункта строки меню (nullptr — нет) / The open menu of a menu bar item (nullptr — none)
    ui::Node* openBarMenu(const RouterState& state) noexcept;

    // ---- splitter_input.cpp ----

    // Длина первой панели разделителя в пределах ручки; true — положение изменилось / The splitter first pane length within the handle limits; true — the position changed
    bool setSplit(ui::Node& splitter, float first);

    // Перетаскивание ручки мышью: точка окна → новая длина первой панели / Dragging the handle by the mouse: the window point → the new first pane length
    void dragSplitter(RouterState& state, ui::Node& handle, Vec2 point);

    // Клавиши на ручке в фокусе (стрелки, Home, End, Enter); true — обработано / The keys on the focused handle (the arrows, Home, End, Enter); true — handled
    bool splitterKey(RouterState& state, ui::Node& handle, const platform::Event& event);

    // Свернуть или развернуть панель collapse; true — состояние сменилось / Collapsing or expanding the collapse pane; true — the state changed
    bool toggleSplit(RouterState& state, ui::Node& splitter);

    // ---- splitter_events.cpp ----

    // Нажатие на ручку: двойной щелчок — свернуть, одиночный — начать перетаскивание / A press on the handle: a double click — collapse, a single one — start dragging
    void pressSplitter(RouterState& state, ui::Node& handle, const platform::Event& event);

    // Начать перетаскивание ползунка или ручки разделителя (нажатие левой кнопкой) / Starting to drag a slider or a splitter handle (a left button press)
    void pressDragControl(RouterState& state, ui::Node& control, const platform::Event& event);

    // onChange разделителя (конец перетаскивания, клавиша, сворачивание) / The splitter onChange (the drag end, a key, collapsing)
    void finishSplit(RouterState& state, ui::Node& splitter);

    // Движение мыши при перетаскивании ползунка или ручки / A mouse move while dragging a slider or a handle
    void moveDragControl(RouterState& state, ui::Node& control, Vec2 point);

    // Конец перетаскивания ползунка или ручки (отпускание кнопки) / The end of dragging a slider or a handle (the button release)
    void endDragControl(RouterState& state);

    // ---- tab_input.cpp ----

    // Запрос закрыть вкладку заголовка: onClose у Tabs со страницей / A request to close the header tab: onClose of Tabs with the page
    void requestTabClose(RouterState& state, ui::Node& header);

    // Ctrl+Tab, Ctrl+Shift+Tab — соседняя вкладка; Ctrl+W, Ctrl+F4 — закрыть выбранную; true — обработано
    // Ctrl+Tab, Ctrl+Shift+Tab — the neighbor tab; Ctrl+W, Ctrl+F4 — close the selected one; true — handled
    bool tabsKey(RouterState& state, const platform::Event& event);

    // Пункт меню вкладок выбирает свою вкладку; false — не пункт меню вкладок / A tab menu item chooses its tab; false — not a tab menu item
    bool chooseTabMirror(RouterState& state, ui::Node& item);

    // ---- tab_drag.cpp ----

    // Нажатие на заголовок: крестик — закрыть, иначе начать перестановку (reorderable) / A press on a header: the cross — close, otherwise start reordering (reorderable)
    void pressTab(RouterState& state, ui::Node& header, const platform::Event& event);

    // Перетаскивание заголовка вдоль полосы / Dragging a header along the strip
    void dragTab(RouterState& state, ui::Node& header, Vec2 point);

    // Конец перетаскивания заголовка: onReorder, если вкладка сдвинулась / The end of a header drag: onReorder if the tab moved
    void finishTabDrag(RouterState& state, ui::Node& header);

    // Нажатие средней кнопкой: закрыть вкладку под указателем / A middle button press: close the tab under the pointer
    void middleClick(RouterState& state, ui::Node* hit);

    // ---- context_menu.cpp ----

    // Встроенное контекстное меню (Копировать, Вставить, Выделить всё) — скрытый слой последним ребёнком корня
    // The built-in context menu (Copy, Paste, Select all) is a hidden layer as the last root child
    ui::Node* createContextMenu(ui::Node& root);

    // Правый щелчок: по тексту или полю — каретка/выделение и контекстное меню в точке / A right click: on a text or a field — the caret/selection and the context menu at the point
    void contextClick(RouterState& state, const platform::Event& event);

    // Встроенная команда пункта контекстного меню; false — пункт не из него / The built-in command of a context menu item; false — the item is not from it
    bool runContextCommand(RouterState& state, const ui::Node& item);

    // ---- popup_keys.cpp ----

    // Следующий включённый пункт слоя по кругу на шаг step (вне слоя — первый или последний); nullptr — пунктов нет
    // The next enabled layer item cyclically by step (outside the layer — the first or the last one); nullptr — no items
    ui::Node* stepItem(ui::Node& popup, const ui::Node* current, int step);

    // Клавиши при открытом слое: Esc — закрыть, стрелки — пункты меню и подменю; true — обработано
    // Keys with an open layer: Esc — close, the arrows — the menu items and submenus; true — handled
    bool popupKey(RouterState& state, const platform::Event& event);

    // ---- command_match.cpp ----

    /*
    ** Включённая команда по сочетанию. Порядок: команды focus своего окна, чей within содержит фокус
    ** (самый глубокий within первым); при модальном диалоге — только они и только внутри него; затем window
    ** и global своего окна, затем global других окон (по порядку sources). nullptr — нет такой.
    **
    ** An enabled command by the combination. The order: the focus commands of the own window whose within holds the focus
    ** (the deepest within first); with a modal dialog — only them and only inside it; then the window
    ** and global ones of the own window, then the global ones of the other windows (in the sources order). nullptr — none.
    */
    ui::Command* matchCommand(const std::vector<CommandSource>& sources, const CommandQuery& query);

    // ---- command_keys.cpp ----

    /*
    ** Нажатие как горячая клавиша (после keyDown приложения, до встроенной обработки). Не команда: при открытом
    ** меню или списке, Tab, клавиши, которые элемент в фокусе обрабатывает сам (набор и правка в поле,
    ** стрелки, пробел и Enter у элементов, Ctrl+A/C у выделенного текста). true — команда выполнена.
    **
    ** A press as a shortcut (after the application keyDown, before the built-in handling). Not a command: with an open
    ** menu or list, Tab, the keys the focused control handles itself (typing and editing in a field,
    ** the arrows, space and Enter on controls, Ctrl+A/C on a selected text). true — a command ran.
    */
    bool commandKey(RouterState& state, const platform::Event& event);

    // ---- keyboard_input.cpp ----

    // Клавиши и символы; true — что-то изменилось / Keys and characters; true — something changed
    bool handleKeyboard(RouterState& state, const platform::Event& event);

    // ---- text_editing.cpp ----

    // Правка поля ввода клавишей; true — текст или каретка изменились / Editing an input with a key; true — the text or the caret changed
    bool editText(RouterState& state, ui::Node& input, const platform::Event& event);

    // Вставка текста вместо выделения или в каретку (с учётом maxLength) / Inserting text instead of the selection or at the caret (respecting maxLength)
    bool insertText(RouterState& state, ui::Node& input, const std::string& text);

    // ---- text_pointer.cpp ----

    // Цель выделения под мышью: поле ввода или Text вне элемента управления; nullptr — нет
    // The selection target under the mouse: an input or a Text outside a control; nullptr — none
    ui::Node* textTarget(ui::Node* hit, ui::Node* control) noexcept;

    // Нажатие в тексте: каретка в точку (Shift — продление), двойной щелчок — слово
    // A press in the text: the caret to the point (Shift — extending), a double click — a word
    void beginTextSelection(RouterState& state, ui::Node& node, const platform::Event& event);

    // Перетаскивание: каретка за мышью, якорь на месте / Dragging: the caret follows the mouse, the anchor stays
    void dragTextSelection(RouterState& state, Vec2 point);

    // Снять выделение с узла selection / Removing the selection from the selection node
    void clearSelection(RouterState& state);

    // Выделить весь текст узла и сделать его узлом выделения / Selecting the whole node text and making it the selection node
    void selectAll(RouterState& state, ui::Node& node);

    // ---- text_navigation.cpp ----

    // Стрелки, Home/End (с Shift — выделение), Ctrl+A, Ctrl+C для Text и поля ввода; true — обработано
    // The arrows, Home/End (with Shift — selecting), Ctrl+A, Ctrl+C for a Text and an input; true — handled
    bool navigateText(RouterState& state, ui::Node& node, const platform::Event& event);
}

#endif
