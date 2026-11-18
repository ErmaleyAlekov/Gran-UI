#include "gran/ui/data_grid.hpp"

namespace gran::ui
{
    // Подъём начинается с родителя: так константный узел даёт изменяемую таблицу без снятия const / The climb starts at the parent: this way a constant node gives a mutable table without casting const away
    Node* gridOf(const Node& node) noexcept
    {
        for (Node* current = node.parent; current != nullptr; current = current->parent)
        {
            if (current->kind == ElementKind::DATA_GRID && current->content.grid)
                return current;
        }
        return nullptr;
    }

    // Первый ребёнок таблицы — строка заголовков (buildDataGrid) / The first table child is the header row (buildDataGrid)
    Node* gridHeader(Node& grid) noexcept
    {
        if (grid.kind != ElementKind::DATA_GRID || grid.children.empty()
            || grid.children.front()->kind != ElementKind::GRID_HEADER)
            return nullptr;
        return grid.children.front().get();
    }

    // Прокрутка — единственный ребёнок таблицы вида SCROLL (перед ней может стоять закреплённый список), в ней — список строк / The scroll is the only table child of the SCROLL kind (a pinned list may stand before it), inside it — the row list
    Node* gridRows(Node& grid) noexcept
    {
        if (grid.kind != ElementKind::DATA_GRID)
            return nullptr;
        for (const std::unique_ptr<Node>& child : grid.children)
        {
            if (child->kind == ElementKind::SCROLL && !child->children.empty())
                return child->children.front().get();
        }
        return nullptr;
    }

    // Подъём от hit до прямого ребёнка строки; его место среди детей строки — номер столбца / The climb from hit to a direct row child; its place among the row children is the column index
    std::size_t cellColumn(const Node& row, const Node& hit) noexcept
    {
        for (const Node* current = &hit; current != nullptr && current != &row; current = current->parent)
        {
            if (current->parent != &row)
                continue;
            for (std::size_t index = 0; index < row.children.size(); ++index)
            {
                if (row.children[index].get() == current)
                    return index;
            }
        }
        return NO_COLUMN;
    }
}
