#ifndef GRAN_A11Y_ACCESS_TREE_HPP
#define GRAN_A11Y_ACCESS_TREE_HPP

#include "gran/ui/node.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gran::a11y
{
    /*
    ** Доступность (экранные дикторы, автотесты интерфейса). Нейтральный слой: роли, имена
    ** и навигация по дереву узлов без знания ОС. Платформенный провайдер (UI Automation на Windows,
    ** позже AT-SPI и NSAccessibility) только переводит эти данные в свой API — паттерн Adapter.
    **
    ** Accessibility (screen readers, UI test automation). The neutral layer: roles, names
    ** and navigation over the node tree without knowing the OS. The platform provider (UI Automation on Windows,
    ** later AT-SPI and NSAccessibility) only translates this data into its API — the Adapter pattern.
    */

    // Роль узла для вспомогательных технологий / The node role for assistive technologies
    enum class Role : std::uint8_t
    {
        GROUP, SCROLL, TEXT, IMAGE, BUTTON, EDIT, CHECK_BOX, SLIDER, MENU, MENU_ITEM, DIALOG, COMBO_BOX,
        RADIO_BUTTON, TAB, TAB_ITEM, PROGRESS_BAR, MENU_BAR, TOOL_BAR, SEPARATOR, STATUS_BAR, THUMB, PANE,
        // Пункт 2.2.6: таблица и её строка заголовков, заголовок столбца, строка и ячейка таблицы, дерево и его строка
        // Item 2.2.6: a table and its header row, a column header, a table row and cell, a tree and its row
        DATA_GRID, HEADER, HEADER_ITEM, DATA_ITEM, CELL, TREE, TREE_ITEM
    };

    /*
    ** Раскрытие узла: NONE — не раскрывается; LEAF — строка дерева без детей; COLLAPSED и EXPANDED — пункт меню
    ** с подменю или строка дерева с веткой.
    **
    ** The node expansion: NONE — it does not expand; LEAF — a tree row without children; COLLAPSED and EXPANDED — a menu item
    ** with a submenu or a tree row with a branch.
    */
    enum class ExpandState : std::uint8_t { NONE, LEAF, COLLAPSED, EXPANDED };

    // Сведения об узле: роль, имя, значение и состояние / The node facts: the role, the name, the value and the state
    struct AccessibleInfo
    {
        Role role = Role::GROUP;
        std::string name;
        std::string value;
        std::string help;
        // Клавиша доступа пункта меню: «Alt+Ф» у пункта строки меню, «Ф» в меню; пусто — нет
        // The menu item access key: "Alt+F" for a menu bar item, "F" in a menu; empty — none
        std::string accessKey;
        bool focusable = false;
        bool enabled = true;
        // Отметка флажка, выбор радиокнопки и вкладки, выделение строки таблицы и дерева / The check box mark, the radio button and tab choice, the table and tree row selection
        bool checked = false;
        ExpandState expand = ExpandState::NONE;
        // Только оформление (контейнер без своего смысла): дикторы его пропускают
        // Only layout (a container without its own meaning): screen readers skip it
        bool decorative = false;
        // Значение меняет только код (поле readOnly, ComboBox, полоса прогресса) / Only the code changes the value (a readOnly input, a ComboBox, a progress bar)
        bool readOnly = false;
        float minimum = 0.0f;
        float maximum = 0.0f;
        float current = 0.0f;
        // Малый шаг (стрелка) и большой (десять малых) диапазона / The small step (an arrow) and the large one (ten small ones) of the range
        float smallStep = 0.0f;
        float largeStep = 0.0f;
    };

    /*
    ** Шаблоны управления узла (биты): что клиент доступности может сделать с элементом —
    ** нажать, переключить, выбрать, задать текст, задать число в диапазоне.
    **
    ** The node control patterns (bits): what an accessibility client can do with the element —
    ** press, toggle, select, set the text, set a number in the range.
    */
    enum Pattern : std::uint16_t
    {
        PATTERN_INVOKE = 1,
        PATTERN_TOGGLE = 2,
        PATTERN_SELECT = 4,
        PATTERN_TEXT = 8,
        PATTERN_RANGE = 16,
        // Раскрыть и свернуть (пункт с подменю, строка дерева) / Expand and collapse (an item with a submenu, a tree row)
        PATTERN_EXPAND = 32,
        // Сетка и таблица: ячейка по строке и столбцу, заголовки столбцов / A grid and a table: a cell by the row and the column, the column headers
        PATTERN_GRID = 64,
        // Ячейка сетки: свои строка и столбец, заголовок столбца / A grid cell: its row and column, the column header
        PATTERN_GRID_ITEM = 128,
        // Контейнер выделения: выделенные строки, можно ли выделить несколько / A selection container: the selected rows, whether several can be selected
        PATTERN_SELECTION = 256
    };

    // Действие, которое просит выполнить клиент доступности / The action an accessibility client asks to perform
    // ADD_SELECT и REMOVE_SELECT — добавить строку к выделению и снять с неё (как щелчок с Ctrl)
    // ADD_SELECT and REMOVE_SELECT — add a row to the selection and drop it (as a click with Ctrl)
    enum class Request : std::uint8_t
    {
        FOCUS, INVOKE, TOGGLE, SELECT, SET_TEXT, SET_VALUE, EXPAND, COLLAPSE, ADD_SELECT, REMOVE_SELECT,
        // Прокрутить список node к записи index (строка вне видимой части виртуального списка) / Scroll the node list to the record index (a row outside the visible part of a virtual list)
        REALIZE
    };

    // «Не строка списка» / "Not a list row"
    constexpr std::size_t NO_ITEM = static_cast<std::size_t>(-1);

    /*
    ** Сведения о строках списков, которых нет в узлах: номер записи, сколько записей всего (у виртуального списка
    ** копии есть лишь у части), режим выделения, глубина строки дерева. Их знает привязка списков, а слой
    ** доступности о привязке не знает — приложение даёт ему этот интерфейс (паттерн Bridge). Вызывается из провайдера
    ** ОС, поэтому noexcept; list — список строк таблицы или дерева, item — копия строки.
    **
    ** The facts about list rows that the nodes do not have: the record index, how many records there are (a virtual list
    ** has copies only for a part), the selection mode, the tree row depth. The list binding knows them while the
    ** accessibility layer does not know the binding — the application gives it this interface (the Bridge pattern). It is
    ** called from the OS provider, hence noexcept; list is the table or tree row list, item is a row copy.
    */
    class ItemSource
    {
    public:
        virtual ~ItemSource() = default;

        // Номер записи строки; NO_ITEM — узел не копия строки списка / The row record index; NO_ITEM — the node is not a list row copy
        virtual std::size_t itemIndex(const ui::Node& item) const noexcept = 0;

        // Записей в списке (вместе с теми, у кого нет копий) / The records in the list (together with those without copies)
        virtual std::size_t itemCount(const ui::Node& list) const noexcept = 0;

        // Копия записи index (nullptr — запись вне окна виртуального списка или её нет) / The copy of the record index (nullptr — outside the virtual list window or none)
        virtual const ui::Node* itemAt(const ui::Node& list, std::size_t index) const noexcept = 0;

        // Выделение списка допускает несколько строк / The list selection allows several rows
        virtual bool multipleSelection(const ui::Node& list) const noexcept = 0;

        // Глубина строки плоского дерева (0 — верхний уровень, и вне дерева) / The flat tree row depth (0 — the top level, and outside a tree)
        virtual std::size_t itemDepth(const ui::Node& item) const noexcept = 0;

        /*
        ** Версия состава и порядка записей списка: при той же версии номер записи указывает на ту же запись
        ** (прокрутка версию не меняет, вставка, удаление, сортировка, раскрытие ветки — меняют).
        **
        ** The list record set and order version: with the same version a record index points to the same record
        ** (scrolling does not change the version, an insertion, a removal, sorting, expanding a branch do).
        */
        virtual std::uint64_t itemVersion(const ui::Node& list) const noexcept = 0;
    };

    /*
    ** Запрос клиента: вид, узел и данные (текст для SET_TEXT, число для SET_VALUE). Провайдер ОС
    ** только кладёт запрос в очередь — менять дерево внутри вызова ОС нельзя (обработчики
    ** приложения могут бросить исключение или удалить узлы); выполняет его цикл приложения.
    **
    ** A client request: the kind, the node and the data (the text for SET_TEXT, the number for SET_VALUE). The OS provider
    ** only queues the request — changing the tree inside an OS call is not allowed (the application
    ** handlers may throw or remove nodes); the application loop performs it.
    */
    struct AccessRequest
    {
        Request kind = Request::FOCUS;
        const ui::Node* node = nullptr;
        std::string text;
        float value = 0.0f;
        // Номер записи списка (REALIZE) / The list record index (REALIZE)
        std::size_t index = 0;
    };

    /*
    ** Общее состояние доступности окна: корень дерева и узел в фокусе. Владеет приложение,
    ** провайдер ОС читает его в потоке интерфейса. root = nullptr — дерево отключено
    ** (провайдеры, ещё живущие у клиентов, отвечают «элемент недоступен»).
    **
    ** The window accessibility state: the tree root and the focused node. The application owns it,
    ** the OS provider reads it on the interface thread. root = nullptr — the tree is disconnected
    ** (the providers still held by clients answer "the element is not available").
    */
    struct AccessTree
    {
        ui::Node* root = nullptr;
        const ui::Node* focus = nullptr;
        // Запросы клиентов, ждущие цикла приложения / The client requests waiting for the application loop
        std::vector<AccessRequest> requests;
        // Сведения о строках списков (nullptr — нет: число строк — только копии) / The list row facts (nullptr — none: the row count is only the copies)
        const ItemSource* items = nullptr;
        /*
        ** Ближайший общий контейнер узлов, удалённых с прошлого кадра (nullptr — удалений не было): провайдер ОС сообщает
        ** клиентам, что его дети сменились, приложение сбрасывает поле после кадра. Только сравнивается и проверяется
        ** contains до разыменования.
        **
        ** The nearest common container of the nodes removed since the previous frame (nullptr — no removals): the OS provider tells
        ** the clients its children changed, the application resets the field after the frame. It is only compared and checked
        ** by contains before dereferencing.
        */
        const ui::Node* changed = nullptr;
        /*
        ** Копии строк, построенные с прошлого кадра (не больше ADDED_LIMIT; дальше — только общий контейнер): клиент мог
        ** запомнить строку «без детей», пока она была вне экрана, — провайдер просит перечитать её детей. Указатели
        ** только сравниваются и проверяются contains до разыменования; приложение очищает список после кадра.
        **
        ** The row copies built since the previous frame (at most ADDED_LIMIT; beyond — only the common container): a client may
        ** have remembered a row "without children" while it was offscreen — the provider asks to reread its children. The pointers
        ** are only compared and checked by contains before dereferencing; the application clears the list after the frame.
        */
        std::vector<const ui::Node*> added;
    };

    // Сколько построенных строк за кадр сообщается поштучно (окно строк при прыжке прокрутки — десятки) / How many built rows per frame are reported one by one (the row window on a scroll jump is dozens)
    constexpr std::size_t ADDED_LIMIT = 128;

    // Направление обхода дерева доступности / The accessibility tree walk direction
    enum class Direction : std::uint8_t { PARENT, NEXT, PREVIOUS, FIRST_CHILD, LAST_CHILD };

    // ---- a11y_describe.cpp ----

    // Роль по виду элемента / The role by the element kind
    Role roleOf(const ui::Node& node) noexcept;

    // Все сведения об узле (пароль не раскрывается) / All the node facts (a password is not disclosed)
    AccessibleInfo describe(const ui::Node& node);

    // Клавиша доступа пункта меню («Alt+Ф», «Ф»; пусто — нет) / The menu item access key ("Alt+F", "F"; empty — none)
    std::string accessKeyOf(const ui::Node& node);

    // Описанный прямоугольник узла в логических координатах окна (с преобразованиями) / The node bounding rectangle in logical window coordinates (with transforms)
    Rect boundsOf(const ui::Node& node) noexcept;

    // ---- a11y_name.cpp ----

    /*
    ** Имя: accessibleName, текст узла labeledBy, затем подпись, текст, подсказка поля или tooltip.
    ** The name: accessibleName, the text of the labeledBy node, then the label, the text, the input placeholder or the tooltip.
    */
    std::string accessibleName(const ui::Node& node);

    // ---- a11y_patterns.cpp ----

    // Шаблоны управления узла (биты Pattern) / The node control patterns (Pattern bits)
    std::uint16_t patternsOf(const ui::Node& node) noexcept;

    // Узел принимает фокус ввода / The node takes the input focus
    bool isFocusable(const ui::Node& node) noexcept;

    // Значение узла меняет только код / Only the code changes the node value
    bool isReadOnly(const ui::Node& node) noexcept;

    // Запрос в очередь дерева; false — не хватило памяти (вызывается из провайдера ОС, поэтому noexcept)
    // A request into the tree queue; false — out of memory (called from the OS provider, hence noexcept)
    bool postRequest(AccessTree& tree, const AccessRequest& request) noexcept;

    // ---- a11y_collection.cpp: таблица и дерево (пункт 2.2.6) / a table and a tree (item 2.2.6) ----

    // Таблица или дерево, чья строка (ячейка, заголовок) — узел; nullptr — узел не из них / The table or the tree whose row (cell, header) the node is; nullptr — none
    const ui::Node* collectionOf(const ui::Node& node) noexcept;

    // Список строк таблицы или дерева (nullptr — не они или части не построены) / The table or tree row list (nullptr — not them or the parts are not built)
    const ui::Node* itemListOf(const ui::Node& collection) noexcept;

    // Шаблоны таблицы, дерева, их строк и ячеек / The patterns of a table, a tree, their rows and cells
    std::uint16_t collectionPatterns(const ui::Node& node) noexcept;

    // Выделенные строки таблицы или дерева среди копий (вне окна виртуального списка их не видно) / The selected table or tree rows among the copies (outside the virtual list window they are not seen)
    std::vector<const ui::Node*> selectedItems(const ui::Node& collection);

    // Есть ли среди копий выделенная строка — без выделения памяти, для методов на границе COM / Whether a selected row is among the copies — without allocating, for the methods on the COM boundary
    bool anySelected(const ui::Node& collection) noexcept;

    // ---- a11y_items.cpp ----

    // Выбран ли узел: отметка радиокнопки и вкладки, выделение строки списка / Whether the node is selected: the radio button and tab mark, the list row selection
    bool isSelected(const ui::Node& node) noexcept;

    // Кнопка раскрытия строки или ячейки столбца-дерева (nullptr — нет) / The expander button of a row or of a tree column cell (nullptr — none)
    const ui::Node* itemExpander(const ui::Node& node) noexcept;

    // Раскрытие пункта меню (подменю) или строки дерева (кнопка раскрытия) / The expansion of a menu item (the submenu) or a tree row (the expander)
    ExpandState expandStateOf(const ui::Node& node) noexcept;

    /*
    ** Текст без смысла для диктора: пустой (место под значение, стрелка несортированного столбца) или стрелка сортировки
    ** в заголовке столбца (направление сообщает свойство заголовка). Он не входит в имя родителя и в «вид элементов».
    **
    ** A text meaningless for a screen reader: empty (a place for a value, the arrow of an unsorted column) or the sort arrow
    ** in a column header (a header property reports the direction). It is not part of the parent name and of the "control view".
    */
    bool hiddenText(const ui::Node& node) noexcept;

    // ---- a11y_navigation.cpp ----

    // Соседний видимый узел в направлении direction; nullptr — нет (у корня нет родителя)
    // The neighboring visible node in the direction; nullptr — none (the root has no parent)
    const ui::Node* navigate(const AccessTree& tree, const ui::Node& node, Direction direction) noexcept;

    // ---- a11y_tree.cpp ----

    // Узел всё ещё в дереве и виден (провайдер мог пережить узел) / The node is still in the tree and visible (a provider may outlive the node)
    bool contains(const AccessTree& tree, const ui::Node* node) noexcept;

    // Самый глубокий узел в точке окна (логические пиксели); корень — если мимо всех / The deepest node at the window point (logical pixels); the root — if outside all
    const ui::Node* nodeAt(const AccessTree& tree, Vec2 point);

    // Узел уходит из дерева (вызывается до удаления): его родитель входит в общий контейнер изменений tree.changed
    // The node leaves the tree (called before the removal): its parent joins the common change container tree.changed
    void noteRemoval(AccessTree& tree, const ui::Node& node) noexcept;

    // Узел построен (уже в дереве): в список tree.added и, как удаление, в общий контейнер / The node was built (already in the tree): into tree.added and, as a removal, into the common container
    void noteAddition(AccessTree& tree, const ui::Node& node) noexcept;
}

#endif
