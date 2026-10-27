#ifndef GRAN_A11Y_ACCESS_TREE_HPP
#define GRAN_A11Y_ACCESS_TREE_HPP

#include "gran/ui/node.hpp"

#include <cstdint>
#include <string>

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
        GROUP, SCROLL, TEXT, IMAGE, BUTTON, EDIT, CHECK_BOX, SLIDER, MENU, MENU_ITEM, DIALOG, COMBO_BOX
    };

    // Сведения об узле: роль, имя, значение и состояние / The node facts: the role, the name, the value and the state
    struct AccessibleInfo
    {
        Role role = Role::GROUP;
        std::string name;
        std::string value;
        std::string help;
        bool focusable = false;
        bool enabled = true;
        bool checked = false;
        // Только оформление (контейнер без своего смысла): дикторы его пропускают
        // Only layout (a container without its own meaning): screen readers skip it
        bool decorative = false;
        float minimum = 0.0f;
        float maximum = 0.0f;
        float current = 0.0f;
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
    };

    // Направление обхода дерева доступности / The accessibility tree walk direction
    enum class Direction : std::uint8_t { PARENT, NEXT, PREVIOUS, FIRST_CHILD, LAST_CHILD };

    // ---- a11y_describe.cpp ----

    // Роль по виду элемента / The role by the element kind
    Role roleOf(const ui::Node& node) noexcept;

    // Имя: подпись, текст, подсказка поля или tooltip / The name: the label, the text, the input placeholder or the tooltip
    std::string accessibleName(const ui::Node& node);

    // Все сведения об узле (пароль не раскрывается) / All the node facts (a password is not disclosed)
    AccessibleInfo describe(const ui::Node& node);

    // Описанный прямоугольник узла в логических координатах окна (с преобразованиями) / The node bounding rectangle in logical window coordinates (with transforms)
    Rect boundsOf(const ui::Node& node) noexcept;

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
