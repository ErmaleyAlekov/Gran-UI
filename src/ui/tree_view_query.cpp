#include "gran/ui/tree_view.hpp"

namespace gran::ui
{
    Node* treeViewRows(Node& view) noexcept
    {
        if (view.kind != ElementKind::TREE_VIEW || view.children.empty())
            return nullptr;
        Node& scroll = *view.children.front();
        if (scroll.kind != ElementKind::SCROLL || scroll.children.empty())
            return nullptr;
        return scroll.children.front().get();
    }

    // Признак ставит привязка списка по плоскому дереву (ListBinding::markTree) / The list binding sets the flag by the flat tree (ListBinding::markTree)
    bool expanderHasBranch(const Node& expander) noexcept
    {
        return expander.kind == ElementKind::TREE_EXPANDER && expander.content.branch;
    }

    /*
    ** Кнопка стоит первой: в строке TreeView — её ребёнок, в таблице-дереве — первый ребёнок ячейки столбца-дерева.
    ** Глубже не ищем: вложенные списки шаблона строки — чужие строки со своими кнопками.
    **
    ** The button stands first: in a TreeView row it is a row child, in a tree table — the first child of the tree column cell.
    ** No deeper search: the nested lists of the row template are other rows with their own buttons.
    */
    Node* rowExpander(Node& row) noexcept
    {
        for (const std::unique_ptr<Node>& child : row.children)
        {
            if (child->kind == ElementKind::TREE_EXPANDER)
                return child.get();
            if (child->kind == ElementKind::GRID_CELL && !child->children.empty()
                && child->children.front()->kind == ElementKind::TREE_EXPANDER)
                return child->children.front().get();
        }
        return nullptr;
    }
}
