#ifndef GRAN_INPUT_INPUT_ROUTER_HPP
#define GRAN_INPUT_INPUT_ROUTER_HPP

#include "gran/platform/window.hpp"
#include "gran/ui/node.hpp"

#include <string>
#include <vector>

namespace gran::input
{
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

    private:
        RouterState _state;
    };

    // ---- hit_test.cpp ----

    // Самый глубокий видимый узел в точке (с учётом отсечения) / The deepest visible node at the point (respecting clipping)
    ui::Node* hitTest(ui::Node& root, Vec2 point);

    // Самый глубокий узел слоя Popup в точке окна / The deepest node of a Popup layer at the window point
    ui::Node* hitLayer(ui::Node& popup, Vec2 point);

    // ---- node_states.cpp ----

    // Бит состояния узлу и всем предкам (hover и pressed в CSS действуют на цепочку) / A state bit to the node and all ancestors
    void setChainState(ui::Node* node, std::uint8_t bit, bool on);

    // Смена узла под мышью, нажатого, в фокусе / Changing the hovered, pressed, focused node
    void changeHover(RouterState& state, ui::Node* node);
    void changePressed(RouterState& state, ui::Node* node);
    void changeFocus(RouterState& state, ui::Node* node);

    // ---- interaction.cpp ----

    // Ближайший предок-элемент управления (кнопка, флажок, ползунок, поле, onClick); nullptr — нет
    // The nearest control ancestor (a button, a check box, a slider, an input, onClick); nullptr — none
    ui::Node* controlOf(ui::Node* node) noexcept;

    // Узел или предок отключён / The node or an ancestor is disabled
    bool disabled(const ui::Node* node) noexcept;

    // Ближайший предок-прокрутка / The nearest scroll ancestor
    ui::Node* scrollOf(ui::Node* node) noexcept;

    // Следующий (или предыдущий) элемент, принимающий фокус, в порядке дерева / The next (or previous) focusable element in tree order
    ui::Node* nextFocusable(ui::Node& root, const ui::Node* current, bool backward);

    // Активация элемента: клик кнопки, переключение флажка (с действиями) / Activating a control: a button click, a check box toggle (with actions)
    void activate(RouterState& state, ui::Node& control);

    // ---- pointer_input.cpp ----

    // События мыши; true — что-то изменилось / Mouse events; true — something changed
    bool handlePointer(RouterState& state, const platform::Event& event);

    // ---- slider_input.cpp ----

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
