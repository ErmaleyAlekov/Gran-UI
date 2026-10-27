#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        /*
        ** Все RadioButton дерева с данным именем группы. Обход от корня: группа с именем, как
        ** name у <input type="radio">, не зависит от того, где кнопки стоят в разметке.
        **
        ** All tree RadioButtons with the given group name. The walk goes from the root: a named group, as
        ** name of <input type="radio">, does not depend on where the buttons stand in the markup.
        */
        void collectNamed(Node& node, const std::string& name, std::vector<Node*>& result)
        {
            if (node.kind == ElementKind::RADIO_BUTTON && node.content.group == name)
                result.push_back(&node);
            for (const std::unique_ptr<Node>& child : node.children)
                collectNamed(*child, name, result);
        }
    }

    // Бит CHECKED включает стиль Checked темы / The CHECKED bit turns on the theme Checked style
    void setChecked(Node& node, bool checked)
    {
        std::uint8_t bit = style::stateBit(style::State::CHECKED);

        if (node.content.checked == checked)
            return;
        node.content.checked = checked;
        if (checked)
            node.stateMask = static_cast<std::uint8_t>(node.stateMask | bit);
        else
            node.stateMask = static_cast<std::uint8_t>(node.stateMask & ~bit);
        markDirty(node, DIRTY_STYLE | DIRTY_PAINT);
    }

    // Безымянная группа и заголовки вкладок — соседи того же вида у одного родителя
    // An unnamed group and tab headers are the siblings of the same kind under one parent
    std::vector<Node*> choiceGroup(Node& member)
    {
        std::vector<Node*> result;
        Node* root = &member;

        if (member.kind != ElementKind::RADIO_BUTTON && member.kind != ElementKind::TAB_HEADER)
            return result;
        if (member.kind == ElementKind::RADIO_BUTTON && !member.content.group.empty())
        {
            while (root->parent != nullptr)
                root = root->parent;
            collectNamed(*root, member.content.group, result);
            return result;
        }
        if (member.parent == nullptr)
            return {&member};
        for (const std::unique_ptr<Node>& child : member.parent->children)
        {
            if (child->kind == member.kind && child->content.group.empty())
                result.push_back(child.get());
        }
        return result;
    }

    // Уже отмеченный член — выбор не меняется (повторный щелчок по радиокнопке её не снимает)
    // An already marked member — the choice does not change (clicking a radio button again does not clear it)
    bool selectInGroup(Node& member)
    {
        if (member.content.checked)
            return false;
        for (Node* other : choiceGroup(member))
            setChecked(*other, other == &member);
        setChecked(member, true);
        return true;
    }

    // Обход группы ищет хотя бы одного отмеченного / The group walk looks for at least one marked member
    bool choiceTabStop(Node& member)
    {
        if (member.content.checked)
            return true;
        for (const Node* other : choiceGroup(member))
        {
            if (other->content.checked)
                return false;
        }
        return true;
    }
}
