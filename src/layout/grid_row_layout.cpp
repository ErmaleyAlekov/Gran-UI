#include "gran/layout/internal/layout_internal.hpp"
#include "gran/ui/data_grid.hpp"

#include <algorithm>

namespace gran::layout::internal
{
    namespace
    {
        // Ширины столбцов таблицы строки; nullptr — строка вне таблицы или ширины ещё не посчитаны / The table column widths of the row; nullptr — the row is outside a table or the widths are not computed yet
        const std::vector<float>* rowWidths(const ui::Node& row) noexcept
        {
            ui::Node* grid = ui::gridOf(row);

            if (grid == nullptr || grid->content.grid->widths.empty())
                return nullptr;
            return &grid->content.grid->widths;
        }

        // Ширина ячейки index; лишняя ячейка (больше, чем столбцов) получает ноль / The width of the cell index; an extra cell (more than the columns) gets zero
        float widthAt(const std::vector<float>& widths, std::size_t index) noexcept
        {
            if (index >= widths.size())
                return 0.0f;
            return widths[index];
        }
    }

    // Строка: ширина — сумма столбцов, высота — наибольшая ячейка при ширине своего столбца / A row: the width is the column sum, the height is the tallest cell at its column width
    Size measureGridRow(ui::Node& row, Size contentAvailable, LayoutPass& pass)
    {
        const std::vector<float>* widths = rowWidths(row);
        Size size{0.0f, 0.0f};

        if (widths == nullptr)
            return measureStack(row, contentAvailable, pass);
        for (std::size_t index = 0; index < row.children.size(); ++index)
        {
            ui::Node& cell = *row.children[index];
            if (!ui::inFlow(cell))
                continue;
            Size cellSize = measureNode(cell, Size{widthAt(*widths, index), contentAvailable.height}, pass);
            size.height = std::max(size.height, cellSize.height);
        }
        for (float width : *widths)
            size.width += width;
        return size;
    }

    // Ячейки встают слева направо по ширинам столбцов на всю высоту строки / The cells stand from left to right by the column widths over the whole row height
    void arrangeGridRow(ui::Node& row, Rect content, LayoutPass& pass)
    {
        const std::vector<float>* widths = rowWidths(row);
        float x = content.x;

        if (widths == nullptr)
        {
            arrangeStack(row, content, pass);
            return;
        }
        for (std::size_t index = 0; index < row.children.size(); ++index)
        {
            ui::Node& cell = *row.children[index];
            float width = widthAt(*widths, index);
            if (ui::inFlow(cell))
                arrangeNode(cell, Rect{x, content.y, width, content.height}, pass);
            x += width;
        }
    }
}
