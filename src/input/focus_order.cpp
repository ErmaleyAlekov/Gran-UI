#include "gran/input/input_router.hpp"

#include <vector>

namespace gran::input
{
    namespace
    {
        // Видимые и включённые элементы, принимающие фокус, в порядке дерева / The visible and enabled focusable controls in tree order
        void collectFocusable(ui::Node& node, std::vector<ui::Node*>& result)
        {
            bool focusable = node.kind == ui::ElementKind::BUTTON || node.kind == ui::ElementKind::CHECKBOX
                || node.kind == ui::ElementKind::SLIDER || ui::isTextField(node)
                || node.kind == ui::ElementKind::MENU_ITEM;

            if (!node.visible || !node.enabled)
                return;
            if (focusable)
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
