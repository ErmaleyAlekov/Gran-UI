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

    // Второй ребёнок — прокрутка, в ней единственный ребёнок — список строк / The second child is the scroll, its only child is the row list
    Node* gridRows(Node& grid) noexcept
    {
        if (grid.kind != ElementKind::DATA_GRID || grid.children.size() < 2)
            return nullptr;
        Node& scroll = *grid.children[1];
        if (scroll.kind != ElementKind::SCROLL || scroll.children.empty())
            return nullptr;
        return scroll.children.front().get();
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
