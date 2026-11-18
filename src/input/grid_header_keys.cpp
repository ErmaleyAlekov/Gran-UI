#include "gran/input/input_router.hpp"
#include "gran/ui/data_grid.hpp"

namespace gran::input
{
    namespace
    {
        // Шаг ширины столбца клавишей (логические пиксели) / The column width step by a key (logical pixels)
        constexpr float GRID_KEY_STEP = 10.0f;

        // Соседний столбец по стрелке в пределах таблицы; NO_COLUMN — край / The neighbor column by the arrow within the table; NO_COLUMN — the edge
        std::size_t neighbor(std::size_t column, std::size_t count, bool right) noexcept
        {
            if (right && column + 1 < count)
                return column + 1;
            if (!right && column > 0)
                return column - 1;
            return ui::NO_COLUMN;
        }
    }

    /*
    ** То же, что мышью, — с клавиатуры (доступность): Ctrl+стрелка переставляет столбец (фокус остаётся на том же
    ** заголовке — он переезжает вместе со столбцом), Shift+стрелка меняет ширину на GRID_KEY_STEP.
    **
    ** The same as by the mouse — from the keyboard (accessibility): Ctrl+arrow moves the column (the focus stays on the same
    ** header — it moves together with the column), Shift+arrow changes the width by GRID_KEY_STEP.
    */
    bool gridHeaderKey(ui::Node& cell, const platform::Event& event)
    {
        ui::Node* grid = ui::gridOf(cell);
        bool control = (event.modifiers & platform::MODIFIER_CONTROL) != 0;
        bool shift = (event.modifiers & platform::MODIFIER_SHIFT) != 0;
        bool right = event.key == platform::Key::RIGHT;

        if (grid == nullptr || cell.parent == nullptr || (!right && event.key != platform::Key::LEFT) || control == shift)
            return false;
        ui::GridState& table = *grid->content.grid;
        std::size_t column = ui::cellColumn(*cell.parent, cell);
        if (column >= table.columns.size())
            return false;
        if (control)
            return table.view.reorderable && ui::moveGridColumn(*grid, column, neighbor(column, table.columns.size(), right));
        if (!table.view.resizable || !table.columns[column].resizable || column >= table.widths.size())
            return false;
        float width = table.widths[column] - GRID_KEY_STEP;
        if (right)
            width = table.widths[column] + GRID_KEY_STEP;
        ui::setGridColumnWidth(*grid, column, style::GridTrack{{style::TrackBoundKind::PIXELS, width},
            {style::TrackBoundKind::PIXELS, width}});
        return true;
    }

    // Над заголовком — его элемент управления (подпись внутри заголовка тоже даёт заголовок) / Over a header — its control (a title inside the header gives the header too)
    bool gridEdgeCursor(const RouterState& state) noexcept
    {
        const ui::Node* cell = nullptr;

        if (state.drag != nullptr && state.drag->kind == ui::ElementKind::GRID_HEADER_CELL)
            return state.gridResize != ui::NO_COLUMN;
        cell = controlOf(state.hover);
        return cell != nullptr && gridEdgeAt(*cell, state.pointer) != ui::NO_COLUMN;
    }
}
