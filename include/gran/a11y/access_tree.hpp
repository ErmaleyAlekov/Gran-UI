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
        RADIO_BUTTON, TAB, TAB_ITEM, PROGRESS_BAR, MENU_BAR, TOOL_BAR, SEPARATOR, STATUS_BAR, THUMB
    };

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
        bool checked = false;
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
    enum Pattern : std::uint8_t
    {
        PATTERN_INVOKE = 1,
        PATTERN_TOGGLE = 2,
        PATTERN_SELECT = 4,
        PATTERN_TEXT = 8,
        PATTERN_RANGE = 16,
        // Раскрыть и свернуть (пункт с подменю) / Expand and collapse (an item with a submenu)
        PATTERN_EXPAND = 32
    };

    // Действие, которое просит выполнить клиент доступности / The action an accessibility client asks to perform
    enum class Request : std::uint8_t { FOCUS, INVOKE, TOGGLE, SELECT, SET_TEXT, SET_VALUE, EXPAND, COLLAPSE };

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
    };

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
    std::uint8_t patternsOf(const ui::Node& node) noexcept;

    // Узел принимает фокус ввода / The node takes the input focus
    bool isFocusable(const ui::Node& node) noexcept;

    // Значение узла меняет только код / Only the code changes the node value
    bool isReadOnly(const ui::Node& node) noexcept;

    // Запрос в очередь дерева; false — не хватило памяти (вызывается из провайдера ОС, поэтому noexcept)
    // A request into the tree queue; false — out of memory (called from the OS provider, hence noexcept)
    bool postRequest(AccessTree& tree, const AccessRequest& request) noexcept;

    // ---- a11y_navigation.cpp ----

    // Соседний видимый узел в направлении direction; nullptr — нет (у корня нет родителя)
    // The neighboring visible node in the direction; nullptr — none (the root has no parent)
    const ui::Node* navigate(const AccessTree& tree, const ui::Node& node, Direction direction) noexcept;

    // ---- a11y_tree.cpp ----

    // Узел всё ещё в дереве и виден (провайдер мог пережить узел) / The node is still in the tree and visible (a provider may outlive the node)
    bool contains(const AccessTree& tree, const ui::Node* node) noexcept;

    // Самый глубокий узел в точке окна (логические пиксели); корень — если мимо всех / The deepest node at the window point (logical pixels); the root — if outside all
    const ui::Node* nodeAt(const AccessTree& tree, Vec2 point);
}

#endif
