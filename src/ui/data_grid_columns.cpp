#include "gran/ui/data_grid.hpp"

#include <algorithm>

namespace gran::ui
{
    namespace
    {
        /*
        ** Перенос элемента from на место to со сдвигом тех, что между ними (как перетаскивание в списке):
        ** [a b c d], 0 → 2 даёт [b c a d]. Номер за концом — ничего (ширин может ещё не быть до первой компоновки).
        **
        ** Moving the element from to the place to, shifting the ones between them (as dragging in a list):
        ** [a b c d], 0 → 2 gives [b c a d]. An index past the end — nothing (the widths may not exist before the first layout).
        */
        template <typename T>
        void moveItem(std::vector<T>& items, std::size_t from, std::size_t to)
        {
            if (from >= items.size() || to >= items.size())
                return;
            if (from < to)
                std::rotate(items.begin() + static_cast<std::ptrdiff_t>(from),
                    items.begin() + static_cast<std::ptrdiff_t>(from + 1), items.begin() + static_cast<std::ptrdiff_t>(to + 1));
            else
                std::rotate(items.begin() + static_cast<std::ptrdiff_t>(to),
                    items.begin() + static_cast<std::ptrdiff_t>(from), items.begin() + static_cast<std::ptrdiff_t>(from + 1));
        }

        // Номер столбца после переноса from → to: текущий и отсортированный столбцы идут за своими столбцами / A column index after moving from → to: the current and the sorted columns follow their columns
        std::size_t movedIndex(std::size_t index, std::size_t from, std::size_t to) noexcept
        {
            if (index == from)
                return to;
            if (from < to && index > from && index <= to)
                return index - 1;
            if (to < from && index >= to && index < from)
                return index + 1;
            return index;
        }

        /*
        ** Ячейки строки переставляются; строку нужно разложить заново: размер у неё прежний, и без флага компоновка
        ** взяла бы прошлое размещение (reuseArrangement) — ячейки остались бы на старых местах. Узлы ячеек те же
        ** (переносятся владеющие указатели), поэтому привязки, фокус и состояния остаются при своих ячейках.
        **
        ** The row cells are rearranged; the row must be laid out anew: its size is the same, and without the flag the layout
        ** would take the past placement (reuseArrangement) — the cells would stay at the old places. The cell nodes are the same
        ** (the owning pointers move), so the bindings, the focus and the states stay with their cells.
        */
        void moveCells(Node& line, std::size_t from, std::size_t to)
        {
            moveItem(line.children, from, to);
            markDirty(line, DIRTY_LAYOUT | DIRTY_PAINT);
        }

        // Шаблоны строк списков таблицы (основного и закреплённых): новые копии строк получают новый порядок / The row templates of the table lists (the main and the pinned ones): new row copies get the new order
        void moveTemplates(Node& grid, std::size_t from, std::size_t to)
        {
            for (const std::unique_ptr<Node>& child : grid.children)
            {
                Node* list = child.get();
                if (child->kind == ElementKind::SCROLL && !child->children.empty())
                    list = child->children.front().get();
                if (list->itemTemplate)
                    moveItem(list->itemTemplate->children, from, to);
            }
        }
    }

    // Порядок меняется сразу у всего: описаний, ширин, заголовков, шаблонов и копий строк / The order changes at once for everything: the descriptions, the widths, the headers, the templates and the row copies
    bool moveGridColumn(Node& grid, std::size_t from, std::size_t to)
    {
        GridState* state = grid.content.grid.get();

        if (state == nullptr || from >= state->columns.size() || to >= state->columns.size() || from == to)
            return false;
        // Ширины до первой компоновки могут отставать от столбцов: выравниваются, чтобы перенос шёл у всех векторов (ревью Т-66)
        // The widths may lag behind the columns before the first layout: they are aligned so the move applies to all vectors (review T-66)
        if (!state->widths.empty())
            state->widths.resize(state->columns.size(), 0.0f);
        if (!state->autoWidths.empty())
            state->autoWidths.resize(state->columns.size(), 0.0f);
        moveItem(state->columns, from, to);
        moveItem(state->widths, from, to);
        moveItem(state->autoWidths, from, to);
        state->current = movedIndex(state->current, from, to);
        if (state->sorted != NO_COLUMN)
            state->sorted = movedIndex(state->sorted, from, to);
        for (Node* line : gridLines(grid))
            moveCells(*line, from, to);
        moveTemplates(grid, from, to);
        state->view.customized = true;
        markDirty(grid, DIRTY_LAYOUT | DIRTY_PAINT);
        return true;
    }
}
