#include "gran/ui/data_grid.hpp"

namespace gran::ui
{
    namespace
    {
        // В строке есть текущая ячейка (строка — текущая запись) / The row has a current cell (the row is the current record)
        bool hasCurrentCell(const Node& row) noexcept
        {
            std::uint16_t bit = style::stateBit(style::State::CURRENT);

            for (const std::unique_ptr<Node>& cell : row.children)
            {
                if ((cell->stateMask & bit) != 0)
                    return true;
            }
            return false;
        }
    }

    bool gridColumnEditable(const GridState& state, std::size_t column) noexcept
    {
        if (!state.editable || column >= state.columns.size())
            return false;
        return state.columns[column].editable && !state.columns[column].field.empty();
    }

    // Обход без переполнения: назад — пока номер больше нуля / A walk without an overflow: backward — while the index is above zero
    std::size_t nextEditableColumn(const GridState& state, std::size_t from, bool backward) noexcept
    {
        std::size_t column = from;

        while (true)
        {
            if (backward && column == 0)
                return NO_COLUMN;
            if (backward)
                --column;
            else
                ++column;
            if (column >= state.columns.size())
                return NO_COLUMN;
            if (gridColumnEditable(state, column))
                return column;
        }
    }

    Node* gridCell(Node& row, std::size_t column) noexcept
    {
        if (column >= row.children.size() || row.children[column]->kind != ElementKind::GRID_CELL)
            return nullptr;
        return row.children[column].get();
    }

    void moveGridCurrent(Node& grid, std::size_t column)
    {
        Node* rows = gridRows(grid);

        if (!setGridColumn(grid, column) || rows == nullptr)
            return;
        for (const std::unique_ptr<Node>& row : rows->children)
        {
            if (row->kind == ElementKind::GRID_ROW)
                markCurrentCell(*row, hasCurrentCell(*row));
        }
        revealGridColumn(grid);
    }
}
