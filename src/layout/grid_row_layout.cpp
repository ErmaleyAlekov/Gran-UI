#include "gran/layout/internal/layout_internal.hpp"
#include "gran/ui/data_grid.hpp"

#include <algorithm>

namespace gran::layout::internal
{
    namespace
    {
        // Состояние таблицы строки; nullptr — строка вне таблицы или ширины ещё не посчитаны / The table state of the row; nullptr — the row is outside a table or the widths are not computed yet
        const ui::GridState* rowState(const ui::Node& row) noexcept
        {
            ui::Node* grid = ui::gridOf(row);

            if (grid == nullptr || grid->content.grid->widths.empty())
                return nullptr;
            return grid->content.grid.get();
        }

        // Ширина ячейки index; лишняя ячейка (больше, чем столбцов) получает ноль / The width of the cell index; an extra cell (more than the columns) gets zero
        float widthAt(const std::vector<float>& widths, std::size_t index) noexcept
        {
            if (index >= widths.size())
                return 0.0f;
            return widths[index];
        }

        /*
        ** Видимая ячейка размещается целиком. Ячейке вне окна столбцов (виртуализация) нужна хотя бы верная рамка:
        ** её читают доступность, фокус и подсказки (ревью Т-66). Тот же размер — поддерево сдвигается (reuseArrangement),
        ** иначе ставится только рамка и флаг: содержимое разложится, когда столбец покажется.
        **
        ** A visible cell is placed entirely. A cell outside the column window (virtualization) needs at least a correct frame:
        ** accessibility, the focus and the tooltips read it (review T-66). The same size — the subtree is shifted (reuseArrangement),
        ** otherwise only the frame and the flag are set: the content is laid out once the column shows.
        */
        void placeCell(ui::Node& cell, Rect frame, bool shown, LayoutPass& pass)
        {
            if (shown)
            {
                arrangeNode(cell, frame, pass);
                return;
            }
            if (reuseArrangement(cell, frame))
                return;
            cell.layout.frame = frame;
            cell.dirty = static_cast<std::uint8_t>(cell.dirty | ui::DIRTY_LAYOUT);
        }
    }

    /*
    ** Строка: ширина — сумма столбцов, высота — наибольшая ячейка при ширине своего столбца. Ячейки столбцов вне
    ** окна прокрутки не меряются (виртуализация столбцов): высота строки — по видимым ячейкам.
    **
    ** A row: the width is the column sum, the height is the tallest cell at its column width. The cells of the columns outside
    ** the scroll viewport are not measured (column virtualization): the row height comes from the visible cells.
    */
    Size measureGridRow(ui::Node& row, Size contentAvailable, LayoutPass& pass)
    {
        const ui::GridState* state = rowState(row);
        Size size{0.0f, 0.0f};

        if (state == nullptr)
            return measureStack(row, contentAvailable, pass);
        for (std::size_t index = 0; index < row.children.size(); ++index)
        {
            ui::Node& cell = *row.children[index];
            if (!ui::inFlow(cell) || !ui::gridCellShown(row, index))
                continue;
            Size cellSize = measureNode(cell, Size{widthAt(state->widths, index), contentAvailable.height}, pass);
            size.height = std::max(size.height, cellSize.height);
        }
        for (float width : state->widths)
            size.width += width;
        return size;
    }

    /*
    ** Ячейки встают слева направо по ширинам столбцов на всю высоту строки. Копия списка уже сдвинута прокруткой,
    ** заголовок и закреплённые строки лежат вне прокрутки — их сдвигает сама строка на scrollX. Закреплённые
    ** ячейки возвращаются на scrollX вправо и стоят на месте; ячейки вне окна прокрутки получают только рамку.
    **
    ** The cells stand from left to right by the column widths over the whole row height. A list copy is already shifted by the scroll,
    ** the header and the pinned rows lie outside the scroll — the row itself shifts them by scrollX. The frozen
    ** cells go back right by scrollX and stay in place; the cells outside the scroll viewport get only the frame.
    */
    void arrangeGridRow(ui::Node& row, Rect content, LayoutPass& pass)
    {
        const ui::GridState* state = rowState(row);
        std::size_t frozen = ui::gridFrozen(row);
        float x = content.x;

        if (state == nullptr)
        {
            arrangeStack(row, content, pass);
            return;
        }
        if (!ui::scrolledLine(row))
            x -= state->view.scrollX;
        for (std::size_t index = 0; index < row.children.size(); ++index)
        {
            ui::Node& cell = *row.children[index];
            float width = widthAt(state->widths, index);
            float left = x;
            if (index < frozen)
                left += state->view.scrollX;
            if (ui::inFlow(cell))
                placeCell(cell, Rect{left, content.y, width, content.height}, ui::gridCellShown(row, index), pass);
            x += width;
        }
    }
}
