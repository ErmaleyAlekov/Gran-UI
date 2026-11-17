#include "gran/ui/data_grid.hpp"

namespace gran::ui
{
    namespace
    {
        // Включить или снять бит состояния; смена — пересчёт стиля и перерисовка / Set or drop a state bit; a change means a style recompute and a repaint
        void setState(Node& node, style::State state, bool on)
        {
            std::uint16_t bit = style::stateBit(state);
            std::uint16_t mask = static_cast<std::uint16_t>(node.stateMask & ~bit);

            if (on)
                mask = static_cast<std::uint16_t>(mask | bit);
            if (mask == node.stateMask)
                return;
            node.stateMask = mask;
            markDirty(node, DIRTY_STYLE | DIRTY_PAINT);
        }
    }

    // Ячейки — прямые дети строки в порядке столбцов (шаблон строки из buildDataGrid) / The cells are the direct row children in the column order (the row template from buildDataGrid)
    void markCurrentCell(Node& row, bool currentRow)
    {
        Node* grid = nullptr;

        if (row.kind != ElementKind::GRID_ROW)
            return;
        grid = gridOf(row);
        if (grid == nullptr)
            return;
        for (std::size_t index = 0; index < row.children.size(); ++index)
            setState(*row.children[index], style::State::CURRENT, currentRow && index == grid->content.grid->current);
    }

    // Без столбцов текущего нет; номер за концом прижимается к последнему столбцу / Without columns there is no current one; an index past the end is pressed to the last column
    bool setGridColumn(Node& grid, std::size_t column)
    {
        GridState* state = grid.content.grid.get();

        if (state == nullptr || state->columns.empty())
            return false;
        if (column >= state->columns.size())
            column = state->columns.size() - 1;
        if (column == state->current)
            return false;
        state->current = column;
        return true;
    }

    /*
    ** Заголовок: [0] подпись, [1] стрелка. Стрелка вверх — по возрастанию, вниз — по убыванию (как в проводниках
    ** и таблицах); у прочих заголовков стрелки нет. Смена текста стрелки меняет её ширину — перекомпоновка.
    **
    ** A header: [0] the title, [1] the arrow. The up arrow is ascending, the down arrow is descending (as in file managers
    ** and spreadsheets); the other headers have no arrow. A change of the arrow text changes its width — a relayout.
    */
    void markSortedColumn(Node& grid, std::size_t column, bool descending)
    {
        Node* header = gridHeader(grid);

        if (header == nullptr || !grid.content.grid)
            return;
        grid.content.grid->sorted = column;
        grid.content.grid->descending = descending;
        for (std::size_t index = 0; index < header->children.size(); ++index)
        {
            Node& cell = *header->children[index];
            std::string arrow;
            if (index == column && descending)
                arrow = "▼";
            else if (index == column)
                arrow = "▲";
            setState(cell, style::State::CHECKED, index == column);
            if (cell.children.size() < 2 || cell.children[1]->content.text == arrow)
                continue;
            cell.children[1]->content.text = arrow;
            markDirty(*cell.children[1], DIRTY_LAYOUT | DIRTY_PAINT);
        }
    }

    /*
    ** Записи удалены или изменены: наибольшее содержимое столбцов auto могло уменьшиться — копить его заново.
    ** Вставка и перестановка максимума не уменьшают и его не трогают: при подгрузке и сортировке столбцы не «прыгают».
    **
    ** Records were removed or changed: the largest content of the auto columns may have shrunk — accumulate it anew.
    ** An insertion and a permutation do not lower the maximum and leave it alone: the columns do not "jump" on loading and sorting.
    */
    void forgetGridContent(const Node& rows) noexcept
    {
        Node* grid = gridOf(rows);

        if (grid == nullptr)
            return;
        grid->content.grid->autoWidths.clear();
        markDirty(*grid, DIRTY_LAYOUT);
    }
}
