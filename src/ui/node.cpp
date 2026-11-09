#include "gran/ui/node.hpp"

#include <utility>

namespace gran::ui
{
    // Ребёнок получает ссылку на родителя / The child gets a reference to the parent
    Node& appendChild(Node& parent, std::unique_ptr<Node> child)
    {
        child->parent = &parent;
        parent.children.push_back(std::move(child));
        return *parent.children.back();
    }

    // Позиция за концом — вставка последним / A position past the end — inserting as the last child
    Node& insertChild(Node& parent, std::size_t index, std::unique_ptr<Node> child)
    {
        if (index >= parent.children.size())
            return appendChild(parent, std::move(child));
        child->parent = &parent;
        parent.children.insert(parent.children.begin() + static_cast<std::ptrdiff_t>(index), std::move(child));
        return *parent.children[index];
    }

    // Имена в порядке ElementKind / The names in the ElementKind order
    const char* elementName(ElementKind kind) noexcept
    {
        static const char* const NAMES[] = {"Stack", "Box", "Grid", "Canvas", "Scroll", "Text", "Image",
            "Button", "Input", "CheckBox", "Slider", "TextArea", "Span", "Popup", "MenuItem", "WebView",
            "Dialog", "ComboBox", "RadioButton", "Switch", "ProgressBar", "Tabs", "Tab", "Icon", "MenuBar",
            "ToolBar", "ToolGroup", "Separator", "TabBar", "TabHeader", "ToolBarMore"};

        // Имя на каждый вид: новый вид без имени не соберётся / A name per kind: a new kind without a name does not compile
        static_assert(sizeof(NAMES) / sizeof(NAMES[0]) == static_cast<std::size_t>(LAST_ELEMENT_KIND) + 1, "elementName table");
        return NAMES[static_cast<std::size_t>(kind)];
    }

    // Подъём к корню прекращается на предке, у которого все нужные флаги уже стоят
    // Climbing to the root stops at an ancestor that already has all the needed flags
    void markDirty(Node& node, std::uint8_t flags) noexcept
    {
        std::uint8_t upward = DIRTY_DESCENDANT;

        node.dirty = static_cast<std::uint8_t>(node.dirty | flags);
        if ((flags & DIRTY_LAYOUT) != 0)
            upward = static_cast<std::uint8_t>(upward | DIRTY_LAYOUT | DIRTY_PAINT);
        for (Node* ancestor = node.parent; ancestor != nullptr; ancestor = ancestor->parent)
        {
            if ((ancestor->dirty & upward) == upward)
                break;
            ancestor->dirty = static_cast<std::uint8_t>(ancestor->dirty | upward);
        }
    }

    // Обход поддерева: биты состояний из полей узла / Walking the subtree: the state bits from the node fields
    void applyInitialStates(Node& node) noexcept
    {
        if (!node.enabled)
            node.stateMask = static_cast<std::uint16_t>(node.stateMask | style::stateBit(style::State::DISABLED));
        if (node.content.checked)
            node.stateMask = static_cast<std::uint16_t>(node.stateMask | style::stateBit(style::State::CHECKED));
        for (const std::unique_ptr<Node>& child : node.children)
            applyInitialStates(*child);
    }
}
