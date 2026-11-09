#ifndef GRAN_UI_NODE_HPP
#define GRAN_UI_NODE_HPP

#include "gran/style/style_sheet.hpp"
#include "gran/ui/ui_event.hpp"
#include "gran/utils/geometry.hpp"
#include "gran/vector/shape.hpp"
#include "gran/platform/window_frame.hpp"
#include "gran/undo/undo_stack.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace beresta
{
    class Element;
    class Document;
}

namespace gran::ui
{
    /*
    ** Вид элемента разметки. Виды до SEPARATOR включительно пишутся в разметке; TAB_BAR и TAB_HEADER
    ** строит движок (полоса заголовков <Tabs>), TOOL_MORE — кнопку «ещё» панели инструментов; у них только
    ** стили темы TabBar, TabHeader и ToolBarMore.
    **
    ** The markup element kind. The kinds up to SEPARATOR inclusive are written in the markup; TAB_BAR and TAB_HEADER
    ** are built by the engine (the <Tabs> header strip), TOOL_MORE — the toolbar "more" button; they have only
    ** the theme styles TabBar, TabHeader and ToolBarMore.
    */
    enum class ElementKind : std::uint8_t
    {
        STACK, BOX, GRID, CANVAS, SCROLL, TEXT, IMAGE, BUTTON, INPUT, CHECKBOX, SLIDER, TEXT_AREA, SPAN, POPUP, MENU_ITEM, WEB_VIEW,
        DIALOG, COMBO_BOX, RADIO_BUTTON, SWITCH, PROGRESS_BAR, TABS, TAB, ICON, MENU_BAR,
        TOOL_BAR, TOOL_GROUP, SEPARATOR, TAB_BAR, TAB_HEADER, TOOL_MORE
    };

    // Последний вид, который пишется в разметке / The last kind written in the markup
    constexpr ElementKind LAST_MARKUP_KIND = ElementKind::SEPARATOR;

    // Последний вид вообще: по нему считается число видов (таблицы стилей и имён) / The last kind at all: the kind count is computed from it (the style and name tables)
    constexpr ElementKind LAST_ELEMENT_KIND = ElementKind::TOOL_MORE;

    /*
    ** Клавиша отправки многострочного поля (onSubmit). CONTROL_ENTER: Enter — новая строка,
    ** Ctrl+Enter — отправка (как в редакторах); ENTER: Enter — отправка, Shift+Enter — новая строка
    ** (как в мессенджерах).
    **
    ** The submit key of a multi-line input (onSubmit). CONTROL_ENTER: Enter is a new line,
    ** Ctrl+Enter submits (as in editors); ENTER: Enter submits, Shift+Enter is a new line
    ** (as in messengers).
    */
    enum class SubmitKey : std::uint8_t { CONTROL_ENTER, ENTER };

    // Как вписать картинку в блок / How to fit an image into the box
    enum class ImageFit : std::uint8_t { FILL, CONTAIN, COVER, NONE };

    // Направление прокрутки / The scroll direction
    enum class ScrollDirection : std::uint8_t { VERTICAL, HORIZONTAL, BOTH };

    // Ребёнок панели инструментов при нехватке места: уходит в меню «ещё» по надобности, всегда или никогда
    // A toolbar child when room is short: goes into the "more" menu as needed, always or never
    enum class OverflowMode : std::uint8_t { AS_NEEDED, ALWAYS, NEVER };

    // Флаги «нужно пересчитать» / The "must recompute" flags
    enum DirtyFlag : std::uint8_t
    {
        DIRTY_STYLE = 1,
        DIRTY_LAYOUT = 2,
        DIRTY_PAINT = 4,
        DIRTY_DESCENDANT = 8
    };

    /*
    ** История правок поля ввода (Input, TextArea): свой стек отмены у каждого поля. textSize — длина текста
    ** после последней записанной правки: другая длина значит, что текст сменили в обход правки (код
    ** приложения присвоил content.text), и прежние шаги к нему уже не применимы — история сбрасывается.
    **
    ** The edit history of an input (Input, TextArea): each field has its own undo stack. textSize is the text length
    ** after the last recorded edit: another length means the text was changed bypassing the editing (the application
    ** code assigned content.text), and the former steps no longer apply to it — the history is reset.
    */
    struct TextHistory
    {
        undo::UndoStack steps;
        std::size_t textSize = 0;
    };

    /*
    ** Атрибут разметки со значением-выражением ({…}): имя атрибута и исходный текст. Узел хранит только
    ** текст, а компилирует и вычисляет выражение модуль binding (у интерфейса нет зависимости от XPath).
    **
    ** A markup attribute with an expression value ({…}): the attribute name and the source text. The node keeps only
    ** the text, while the binding module compiles and evaluates the expression (the UI has no dependency on XPath).
    */
    struct AttributeBinding
    {
        std::string attribute;
        std::string source;
    };

    /*
    ** Содержимое элемента, не относящееся к стилю: текст, источник картинки,
    ** значения полей ввода и имена действий. Хранится плоско (без иерархии классов
    ** виджетов): у каждого вида свои поля, остальные пусты.
    **
    ** The element content not related to style: the text, the image source,
    ** input values and action names. Stored flat (without a widget class hierarchy):
    ** each kind uses its own fields, the others stay empty.
    */
    struct NodeContent
    {
        platform::WindowRole windowRole = platform::WindowRole::NONE;
        std::string text;
        std::string placeholder;
        std::string source;
        std::string tooltip;
        // Имя для дикторов (как aria-label) и id узла-подписи (как aria-labelledby) / The name for screen readers (as aria-label) and the id of the label node (as aria-labelledby)
        std::string accessibleName;
        std::string labeledBy;
        std::string onClick;
        std::string onChange;
        std::string onSubmit;
        // Имя модели списка (items="…"): дети строятся из шаблона по записям модели / The list model name (items="…"): children are built from the template by the model records
        std::string items;
        // Модель выделения списка (selection="…"): копии отмеченных записей получают состояние Selected / The list selection model (selection="…"): the copies of the marked records get the Selected state
        std::string selection;
        // Действие на строку из страницы <WebView> (Application::onWebMessage) / The action for a string from a <WebView> page (Application::onWebMessage)
        std::string onMessage;
        // Фигура <Icon data="…"> (из разметки; иконка из файла source — у рендерера) / The <Icon data="…"> shape (from the markup; an icon from the source file is the renderer's)
        std::shared_ptr<const vector::Shape> shape;
        // Действие на перетащенные файлы (Application::onFileDrop): элемент — зона сброса / The action for dragged files (Application::onFileDrop): the element is a drop zone
        std::string onDrop;
        float minimum = 0.0f;
        float maximum = 100.0f;
        float value = 0.0f;
        float step = 0.0f;
        std::size_t maxLength = 0;
        std::size_t caret = 0;
        // Второй конец выделения (байт UTF-8); выделение — между anchor и caret, пусто при равенстве
        // The other selection end (a UTF-8 byte); the selection lies between anchor and caret, empty when equal
        std::size_t anchor = 0;
        // Наименьшее число видимых строк TextArea (как rows у <textarea>) / The least visible line count of a TextArea (as rows of <textarea>)
        std::size_t rows = 2;
        bool checked = false;
        bool password = false;
        bool readOnly = false;
        // Input, TextArea: undo="false" — правки не записываются в историю (Ctrl+Z ничего не отменяет)
        // Input, TextArea: undo="false" — the edits are not recorded in the history (Ctrl+Z undoes nothing)
        bool undoable = true;
        // Виртуальный список (virtual="true"): копии только у записей около видимой области / A virtual list (virtual="true"): copies only for the records near the visible area
        bool virtualize = false;
        ImageFit fit = ImageFit::FILL;
        ScrollDirection direction = ScrollDirection::VERTICAL;
        SubmitKey submitKey = SubmitKey::CONTROL_ENTER;
        // Popup: рамка якоря в координатах окна (нулевая высота — точка); passive — подсказка (не ловит мышь)
        // Popup: the anchor frame in window coordinates (a zero height — a point); passive — a tooltip (does not catch the mouse)
        Rect popupAnchor;
        bool popupPassive = false;
        // Button: id слоя (Popup или Dialog), который кнопка открывает сама (щелчок, Enter, пробел, стрелка вниз)
        // Button: the id of the layer (a Popup or a Dialog) the button opens by itself (a click, Enter, space, the down arrow)
        std::string popup;
        // Button, MenuItem: id команды <Command> (действие, enabled, подпись) и подпись её сочетания для меню и доступности
        // Button, MenuItem: the <Command> id (the action, enabled, the label) and its shortcut label for menus and accessibility
        std::string command;
        std::string shortcut;
        // MenuItem в меню или строке меню: буква доступа из «_Файл» (свёрнутый регистр, 0 — нет) и её смещение
        // в байтах подписи text (подчёркивание рисуется под ней, когда видны подсказки клавиатуры)
        // MenuItem in a menu or a menu bar: the access letter from "_File" (folded case, 0 — none) and its byte
        // offset in the text label (the underline is drawn under it when the keyboard cues are shown)
        char32_t mnemonic = 0;
        std::size_t mnemonicAt = 0;
        // Ребёнок ToolBar: режим overflowMode и ушёл ли он сейчас в меню «ещё» (тогда скрыт компоновкой, а не приложением)
        // A ToolBar child: the overflowMode and whether it went into the "more" menu now (then hidden by the layout, not by the application)
        OverflowMode overflowMode = OverflowMode::AS_NEEDED;
        bool overflowed = false;
        // MenuItem, RadioButton, Tab — значение (value; пусто — текст), ComboBox и Tabs — значение выбранного
        // MenuItem, RadioButton, Tab — the value (value; empty — the text), ComboBox and Tabs — the selected value
        std::string option;
        // RadioButton: имя группы на всё дерево (пусто — группа из соседей без имени) / RadioButton: the group name for the whole tree (empty — a group of unnamed siblings)
        std::string group;
        // Dialog: Esc закрывает диалог (closable="false" — закрыть может только код) / Dialog: Esc closes the dialog (closable="false" — only the code can close it)
        bool closable = true;
        // Обработчики событий из разметки (onPointerDown, onKeyDownCapture, on="своё:действие"…)
        // The event handlers from the markup (onPointerDown, onKeyDownCapture, on="custom:action"…)
        std::vector<EventBinding> events;
        // Перетаскивание узла: источник (draggable, формат и данные) и зона (форматы acceptDrop через пробел, * — любой)
        // Dragging a node: the source (draggable, the format and the data) and the zone (the acceptDrop formats by spaces, * — any)
        bool draggable = false;
        std::string dragFormat;
        std::string dragData;
        std::string acceptDrop;
        // Input, TextArea: история правок для Ctrl+Z / Ctrl+Y; создаётся при первой правке, сбрасывается значением из кода.
        // shared_ptr — ради копирования содержимого (копии шаблонов списков); копия узла историю не делит (copyTree её снимает).
        // Input, TextArea: the edit history for Ctrl+Z / Ctrl+Y; created on the first edit, reset by a value from code.
        // shared_ptr — for copying the content (the list template copies); a node copy does not share the history (copyTree drops it).
        std::shared_ptr<TextHistory> history;
        // Атрибуты с выражениями привязки {…} (копии шаблонов получают их вместе с содержимым)
        // The attributes with {…} binding expressions (the template copies get them together with the content)
        std::vector<AttributeBinding> bindings;
    };

    // Запомненное измерение: доступный размер → результат / A remembered measurement: the available size → the result
    struct MeasureEntry
    {
        Size available;
        Size result;
    };

    /*
    ** Результат компоновки: рамка блока (border box) в координатах окна, размер содержимого
    ** (для прокрутки) и кэш измерений текущего прохода (номер прохода — generation).
    **
    ** The layout result: the border box in window coordinates, the content size
    ** (for scrolling) and the measurement cache of the current pass (the pass number is generation).
    */
    struct LayoutBox
    {
        Rect frame;
        Size contentSize;
        Vec2 scrollOffset;
        MeasureEntry cache[2];
        std::uint32_t generation = 0;
        std::uint8_t cacheCount = 0;
    };

    /*
    ** Узел дерева интерфейса. Родитель владеет детьми (unique_ptr), ребёнок знает
    ** родителя невладеющим указателем — как в «Берёсте», без циклов владения.
    **
    ** A UI tree node. A parent owns its children (unique_ptr), a child knows
    ** its parent through a non-owning pointer — as in Beresta, without ownership cycles.
    */
    struct Node
    {
        ElementKind kind = ElementKind::BOX;
        std::string id;
        const style::StyleRule* rule = nullptr;
        style::DeclarationBlock declared;
        NodeContent content;
        style::ComputedStyle computed;
        LayoutBox layout;
        bool visible = true;
        bool enabled = true;
        bool styled = false;
        std::uint16_t stateMask = 0;
        std::uint8_t dirty = DIRTY_STYLE | DIRTY_LAYOUT | DIRTY_PAINT;
        std::size_t line = 0;
        std::size_t column = 0;
        Node* parent = nullptr;
        /*
        ** Элемент живого DOM документа, из которого построен узел (невладеющий; владеет UiDocument::dom).
        ** nullptr — узел создан кодом, копией шаблона или движком (TabBar, слой ComboBox): в разметке его нет.
        ** Указатель действителен, пока узел в дереве окна: удаление и перестройка узла снимают его вместе с элементом.
        **
        ** The live DOM element of the document the node is built from (non-owning; UiDocument::dom owns it).
        ** nullptr — the node is created by code, as a template copy or by the engine (TabBar, the ComboBox layer): it is not in the markup.
        ** The pointer is valid while the node is in a window tree: removing and rebuilding the node drop it together with the element.
        */
        const beresta::Element* domElement = nullptr;
        /*
        ** Развёртки компонентов, корнем которых стал узел (<c:Card> → тело Card): копии тел с подставленными
        ** параметрами, на их элементы ссылаются узлы тела. Объявлены до children: дети уничтожаются раньше.
        ** Несколько — если корень тела сам экземпляр другого компонента.
        **
        ** The component expansions whose root the node became (<c:Card> → the Card body): the body copies with the parameters
        ** substituted, the body nodes refer to their elements. Declared before children: the children are destroyed earlier.
        ** Several — if the body root is itself an instance of another component.
        */
        std::vector<std::shared_ptr<const beresta::Document>> expansions;
        std::vector<std::unique_ptr<Node>> children;
        // Шаблон одной записи списка (<ItemTemplate>) — вне дерева, только образец для копий / The template of one list record (<ItemTemplate>) — outside the tree, only a sample for copies
        std::unique_ptr<Node> itemTemplate;
        // Слушатели из кода (Application::addEventListener); копия cloneTree их не получает
        // The listeners from code (Application::addEventListener); a cloneTree copy does not get them
        std::vector<EventListener> listeners;
    };

    // Добавляет ребёнка и возвращает его (node.cpp) / Adds a child and returns it (node.cpp)
    Node& appendChild(Node& parent, std::unique_ptr<Node> child);

    // Вставляет ребёнка перед позицией index (за концом — последним) / Inserts a child before the index position (past the end — as the last one)
    Node& insertChild(Node& parent, std::size_t index, std::unique_ptr<Node> child);

    // Имя элемента разметки ("Stack", "Button"...) / The markup element name ("Stack", "Button"...)
    const char* elementName(ElementKind kind) noexcept;

    /*
    ** Помечает узел. Предки получают DIRTY_DESCENDANT (проход стилей найдёт узел, не обходя
    ** чистые ветви), а при DIRTY_LAYOUT — и DIRTY_LAYOUT: размер ребёнка меняет размер родителя.
    **
    ** Marks the node. The ancestors get DIRTY_DESCENDANT (the style pass finds the node without walking
    ** clean branches), and with DIRTY_LAYOUT — DIRTY_LAYOUT too: a child size changes the parent size.
    */
    void markDirty(Node& node, std::uint8_t flags) noexcept;

    // Начальные состояния из разметки: enabled="false" → DISABLED, checked="true" → CHECKED (всё поддерево)
    // The initial states from the markup: enabled="false" → DISABLED, checked="true" → CHECKED (the whole subtree)
    void applyInitialStates(Node& node) noexcept;

    // Кнопка управления окном (не область перетаскивания).
    // A window command button (not a drag region).
    bool isWindowButton(const Node& node) noexcept;

    // Состояние Maximized у всего поддерева; изменившиеся узлы получают DIRTY_STYLE.
    // The Maximized state throughout a subtree; changed nodes receive DIRTY_STYLE.
    void setWindowMaximized(Node& node, bool maximized);

    // ---- node_kinds.cpp ----

    // Поле правки текста: однострочное Input или многострочное TextArea / A text editing field: a single-line Input or a multi-line TextArea
    bool isTextField(const Node& node) noexcept;

    // Узел участвует в компоновке родителя: видим и не всплывающий слой (Popup размещается поверх всего)
    // The node takes part in the parent layout: visible and not a popup layer (a Popup is placed over everything)
    bool inFlow(const Node& node) noexcept;

    // Ближайший всплывающий слой (Popup или Dialog) среди узла и его предков (nullptr — узел не в слое)
    // The nearest popup layer (a Popup or a Dialog) among the node and its ancestors (nullptr — the node is not in a layer)
    Node* popupOf(Node* node) noexcept;

    // Слой поверх дерева: Popup (меню, список, подсказка) или Dialog (модальный) / A layer over the tree: a Popup (a menu, a list, a tooltip) or a Dialog (modal)
    bool isLayer(const Node& node) noexcept;

    // Включить или выключить узел (enabled и состояние DISABLED) / Enabling or disabling the node (enabled and the DISABLED state)
    void setEnabled(Node& node, bool enabled);

    // ---- menu_bar.cpp ----

    /*
    ** Буква доступа из подписи пункта: «_Файл» → подпись «Файл», буква «ф»; «__» — сам знак «_». Без «_»
    ** перед буквой — ничего не меняется (mnemonic = 0). true — буква найдена.
    **
    ** The access letter from the item label: "_File" → the label "File", the letter "f"; "__" is the "_" sign itself.
    ** Without "_" before a letter nothing changes (mnemonic = 0). true — a letter was found.
    */
    bool takeMnemonic(Node& item);

    // Свёртка регистра буквы доступа (Ф и ф, F и f совпадают) / The case folding of an access letter (Ф and ф, F and f match)
    char32_t foldMnemonic(char32_t symbol);

    // Пункт строки меню: MenuItem, ребёнок MenuBar / A menu bar item: a MenuItem that is a MenuBar child
    bool isMenuBarItem(const Node& node) noexcept;

    // Первый видимый включённый пункт-ребёнок container с буквой доступа symbol (nullptr — нет)
    // The first visible enabled item child of container with the access letter symbol (nullptr — none)
    Node* mnemonicItem(Node& container, char32_t symbol);

    // Видимая строка меню окна вне слоёв (первая в порядке обхода; nullptr — нет)
    // The visible window menu bar outside the layers (the first in the walk order; nullptr — none)
    Node* menuBarOf(Node& root);

    // ---- tool_bar.cpp, tool_bar_sync.cpp ----

    // Кнопка «ещё» панели инструментов (nullptr — не панель или кнопки нет) / The toolbar "more" button (nullptr — not a toolbar or no button)
    Node* toolBarMore(Node& bar) noexcept;

    // Добавить в конец панели кнопку «ещё» с меню из пунктов-зеркал всех детей панели (построитель разметки)
    // Appending to the toolbar end the "more" button with a menu of mirror items of all the toolbar children (the markup builder)
    void buildToolBarMore(Node& bar);

    // Видимость пунктов-зеркал — по детям панели, ушедшим в меню (после компоновки панели)
    // The mirror item visibility — by the toolbar children that went into the menu (after the toolbar layout)
    void syncOverflowMenu(Node& bar);

    // Вернуть всех детей из меню «ещё» и скрыть кнопку (замер полной длины панели перед новой раскладкой)
    // Returning all the children from the "more" menu and hiding the button (measuring the full toolbar length before a new layout)
    void unfoldToolBar(Node& bar) noexcept;

    // ---- menu_shortcut.cpp ----

    // Зазор между подписью пункта меню и подписью сочетания — в кеглях / The gap between a menu item label and the shortcut label — in font sizes
    constexpr float SHORTCUT_GAP = 2.0f;

    /*
    ** Подпись сочетания пункта меню (content.shortcut) как отдельный текстовый узел вне дерева: тот же
    ** вычисленный стиль, что у пункта, поэтому компоновка меряет, а сцена рисует его общим текстовым движком.
    **
    ** The menu item shortcut label (content.shortcut) as a separate text node outside the tree: the same
    ** computed style as the item, so the layout measures and the scene paints it with the shared text engine.
    */
    Node shortcutLabel(const Node& item);

    // ---- node_tree.cpp ----

    // Узел поддерева по id, поиск в глубину (nullptr — нет такого) / A subtree node by id, depth-first (nullptr — no such node)
    Node* findNode(Node& subtree, const std::string& id) noexcept;

    /*
    ** Копия поддерева для вставки во время работы (шаблоны в разметке: скрытый образец →
    ** копия → appendChild). Копируются вид, id, стиль, объявления, содержимое, видимость, шаблон списка;
    ** вычисленный стиль, компоновка и состояния — нет: копия получает их заново при вставке.
    **
    ** A subtree copy for inserting at run time (templates in the markup: a hidden sample →
    ** a copy → appendChild). The kind, id, style, declarations, content, visibility, the list template are copied;
    ** the computed style, layout and states are not: the copy gets them anew on insertion.
    */
    std::unique_ptr<Node> cloneTree(const Node& node);

    /*
    ** Оформленный текст: дети Text вида SPAN — участки по порядку (у каждого свой стиль),
    ** текст Text — их склейка (по нему работают выделение, копирование, доступность).
    ** Есть ли у узла участки / Whether the node has runs.
    **
    ** Styled text: the SPAN children of a Text are the runs in order (each with its own style),
    ** the Text text is them joined (selection, copying, accessibility work on it).
    */
    bool hasSpans(const Node& node) noexcept;

    // Текст Text заново из участков (после их правки из кода) и перекомпоновка / The Text text anew from the runs (after editing them from code) and a relayout
    void joinSpans(Node& node);

    // ---- combo_box.cpp ----

    /*
    ** Выпадающий список <ComboBox>: пункты MenuItem (или items + <ItemTemplate>) при построении
    ** переносятся во внутренний слой Popup — последнего ребёнка ComboBox. Выбранное значение — content.option,
    ** подпись выбранного пункта — content.text (её показывает поле), выбранный пункт — в состоянии Checked.
    **
    ** A <ComboBox> drop-down list: the MenuItem items (or items + <ItemTemplate>) are moved at build time
    ** into an inner Popup layer — the last ComboBox child. The selected value is content.option,
    ** the selected item label is content.text (the field shows it), the selected item is in the Checked state.
    */

    // Внутренний слой списка (nullptr — не ComboBox) / The inner list layer (nullptr — not a ComboBox)
    Node* comboList(Node& combo) noexcept;

    // Значение пункта: value, а без него — текст / The item value: value, and without it — the text
    const std::string& optionValue(const Node& item) noexcept;

    // Пункт списка со значением value (nullptr — нет такого) / The list item with the value value (nullptr — none)
    Node* findOption(Node& combo, const std::string& value) noexcept;

    // Выбрать пункт (nullptr — снять выбор): значение, подпись, Checked; true — значение изменилось
    // Selecting an item (nullptr — clearing the choice): the value, the label, Checked; true — the value changed
    bool selectOption(Node& combo, Node* item);

    // ---- choice_group.cpp ----

    // Отметка узла: content.checked, бит CHECKED и перерисовка (если изменилась) / The node mark: content.checked, the CHECKED bit and a repaint (if it changed)
    void setChecked(Node& node, bool checked);

    /*
    ** Группа выбора «один из»: RadioButton с именем group — все такие во всём дереве, без имени —
    ** соседи без имени; TabHeader — заголовки своей полосы. Не член группы — пустой список.
    **
    ** A "one of" choice group: a RadioButton with a group name — all such ones in the whole tree, without a name —
    ** the unnamed siblings; a TabHeader — the headers of its strip. Not a group member — an empty list.
    */
    std::vector<Node*> choiceGroup(Node& member);

    // Отметить member и снять отметку с остальных членов группы; true — выбор изменился
    // Marking member and unmarking the other group members; true — the choice changed
    bool selectInGroup(Node& member);

    // Остановка Tab-обхода: отмеченный член группы, а при пустом выборе — каждый (как в браузерах)
    // A Tab walk stop: the marked group member, and with an empty choice — each one (as in browsers)
    bool choiceTabStop(Node& member);

    // ---- tabs.cpp ----

    /*
    ** Вкладки <Tabs>: дети Tab — страницы, первый ребёнок — полоса заголовков (TabBar с TabHeader по
    ** заголовку на страницу), её строит syncTabs. Видна только выбранная страница; значение — content.option
    ** Tabs (value страницы, без него — её text).
    **
    ** The <Tabs> tabs: the Tab children are the pages, the first child is the header strip (a TabBar with a TabHeader
    ** per page), syncTabs builds it. Only the selected page is visible; the value is content.option of Tabs
    ** (the page value, without it — its text).
    */

    // Полоса заголовков заново по страницам и выбор по content.option (пусто или нет такой — первая)
    // The header strip anew by the pages and the choice by content.option (empty or no such one — the first)
    void syncTabs(Node& tabs);

    // Выбрать страницу со значением value: заголовок, видимость страниц, перекомпоновка; true — выбор изменился
    // Selecting the page with the value value: the header, the page visibility, a relayout; true — the choice changed
    bool selectTab(Node& tabs, const std::string& value);

    // Вкладки заголовка (nullptr — узел не заголовок вкладок) / The tabs of a header (nullptr — the node is not a tab header)
    Node* tabsOf(Node& header) noexcept;

    // ---- layer_tree.cpp ----

    // Подменю пункта MenuItem (вложенный Popup) или nullptr / The submenu of a MenuItem (a nested Popup) or nullptr
    Node* submenuOf(const Node& item) noexcept;

    // Узел и все его предки видимы / The node and all its ancestors are visible
    bool isShown(const Node& node) noexcept;

    // Подписи всех ComboBox поддерева — по их значениям (после построения пунктов из данных)
    // The labels of all subtree ComboBoxes — by their values (after building the items from data)
    void syncComboBoxes(Node& subtree);

    // ---- node_transform.cpp ----

    // CSS-преобразование узла вокруг transformOrigin (тождественное, если transform не задан)
    // The node CSS transform around transformOrigin (the identity if transform is not set)
    Affine2D nodeTransform(const Node& node) noexcept;

    // Из координат компоновки узла в координаты окна (преобразования всех предков и самого узла)
    // From the node layout coordinates into window coordinates (the transforms of all ancestors and the node itself)
    Affine2D nodeToWindow(const Node& node) noexcept;

    // Точка окна → координаты компоновки узла (для попадания, ползунка, текста) / A window point → the node layout coordinates (for hits, sliders, text)
    Vec2 windowToNode(const Node& node, Vec2 point) noexcept;
}

#endif
