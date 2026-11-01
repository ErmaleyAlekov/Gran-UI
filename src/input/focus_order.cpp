#include "gran/input/input_router.hpp"

#include <vector>

namespace gran::input
{
    namespace
    {
        /*
        ** Принимает ли узел фокус Tab. Из группы «один из» (радиокнопки, вкладки) — только отмеченный:
        ** внутри группы ходят стрелками, и Tab переходит к следующему элементу за группой.
        **
        ** Whether the node takes the Tab focus. From a "one of" group (radio buttons, tabs) — only the marked one:
        ** the arrows move inside the group, and Tab goes to the next control after the group.
        */
        bool focusable(ui::Node& node)
        {
            ui::ElementKind kind = node.kind;

            if (kind == ui::ElementKind::RADIO_BUTTON || kind == ui::ElementKind::TAB_HEADER)
                return ui::choiceTabStop(node);
            return kind == ui::ElementKind::BUTTON || kind == ui::ElementKind::CHECKBOX || kind == ui::ElementKind::SWITCH
                || kind == ui::ElementKind::SLIDER || ui::isTextField(node)
                || kind == ui::ElementKind::MENU_ITEM || kind == ui::ElementKind::COMBO_BOX || ui::isWindowButton(node)
                || !node.content.selection.empty();
        }

        // Видимые и включённые элементы, принимающие фокус, в порядке дерева / The visible and enabled focusable controls in tree order
        void collectFocusable(ui::Node& node, std::vector<ui::Node*>& result)
        {
            if (!node.visible || !node.enabled)
                return;
            if (focusable(node))
                result.push_back(&node);
            for (const std::unique_ptr<ui::Node>& child : node.children)
                collectFocusable(*child, result);
        }
    }

    // Tab и Shift+Tab идут по кругу; без текущего — первый или последний / Tab and Shift+Tab go around; without a current one — the first or the last
    ui::Node* nextFocusable(ui::Node& root, const ui::Node* current, bool backward)
    {
        std::vector<ui::Node*> focusable;
        std::size_t position = 0;

        collectFocusable(root, focusable);
        if (focusable.empty())
            return nullptr;
        while (position < focusable.size() && focusable[position] != current)
            ++position;
        if (position == focusable.size())
        {
            if (backward)
                return focusable.back();
            return focusable.front();
        }
        if (backward)
            return focusable[(position + focusable.size() - 1) % focusable.size()];
        return focusable[(position + 1) % focusable.size()];
    }
}
