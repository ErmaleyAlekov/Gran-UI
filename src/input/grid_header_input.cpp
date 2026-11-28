#include "gran/input/input_router.hpp"
#include "gran/ui/data_grid.hpp"

#include <cmath>

namespace gran::input
{
    namespace
    {
        // Полоса у края заголовка, за которую тянут ширину (логические пиксели) / The strip at a header edge the width is dragged by (logical pixels)
        constexpr float GRID_EDGE = 5.0f;

        // Сдвиг, после которого нажатие на заголовок — перестановка, а не щелчок / The shift after which a press on a header is reordering, not a click
        constexpr float GRID_DRAG_THRESHOLD = 4.0f;

        /*
        ** Указатель зашёл за середину видимого соседнего заголовка — столбец встаёт на его место (как вкладки:
        ** за середину — чтобы столбцы разной ширины не менялись туда-обратно на каждом движении).
        **
        ** The pointer went past the middle of a visible neighbor header — the column takes its place (as the tabs:
        ** past the middle — so that columns of different widths do not swap back and forth on every move).
        */
        void reorderHeader(ui::Node& grid, ui::Node& cell, float x)
        {
            ui::Node& header = *cell.parent;
            std::size_t from = ui::cellColumn(header, cell);

            for (std::size_t index = 0; index < header.children.size(); ++index)
            {
                const Rect& frame = header.children[index]->layout.frame;
                float middle = frame.x + frame.width * 0.5f;
                if (index == from || !ui::gridCellVisibleAt(header, index, x))
                    continue;
                bool passed = (index > from && x > middle) || (index < from && x < middle);
                if (passed && x >= frame.x && x < frame.x + frame.width)
                {
                    ui::moveGridColumn(grid, from, index);
                    return;
                }
            }
        }
    }

    /*
    ** Правый край своего заголовка — ширина этого столбца; левый край — ширина предыдущего видимого (граница
    ** общая, и тянуть её удобно с обеих сторон). Таблица или столбец с resizable="false" — края нет.
    **
    ** The right edge of the own header is this column width; the left edge is the width of the previous visible one (the border
    ** is shared, and it is convenient to drag it from both sides). A table or a column with resizable="false" — no edge.
    */
    std::size_t gridEdgeAt(const ui::Node& cell, Vec2 point) noexcept
    {
        const ui::Node* grid = ui::gridOf(cell);

        if (cell.kind != ui::ElementKind::GRID_HEADER_CELL || cell.parent == nullptr || grid == nullptr
            || !grid->content.grid->view.resizable)
            return ui::NO_COLUMN;
        const std::vector<ui::GridColumn>& columns = grid->content.grid->columns;
        float x = ui::pointerToNode(*cell.parent, point).x;
        const Rect& frame = cell.layout.frame;
        std::size_t index = ui::cellColumn(*cell.parent, cell);
        std::size_t column = ui::NO_COLUMN;
        if (frame.x + frame.width - x <= GRID_EDGE)
            column = index;
        else if (x - frame.x <= GRID_EDGE && index > 0 && ui::gridCellShown(*cell.parent, index - 1))
            column = index - 1;
        if (column >= columns.size() || !columns[column].resizable)
            return ui::NO_COLUMN;
        return column;
    }

    /*
    ** У края нажатие не щёлкает (сортировки нет): запоминается ширина столбца, двойной щелчок ставит ширину
    ** по содержимому (auto). Мимо края — перестановка, если таблица разрешает; без сдвига отпускание — щелчок.
    **
    ** At the edge a press does not click (no sorting): the column width is remembered, a double click sets the width
    ** by the content (auto). Away from the edge — reordering if the table allows it; a release without a shift is a click.
    */
    void pressGridHeader(RouterState& state, ui::Node& cell, const platform::Event& event)
    {
        ui::Node* grid = ui::gridOf(cell);
        std::size_t edge = gridEdgeAt(cell, event.position);

        if (grid == nullptr || cell.parent == nullptr)
            return;
        state.gridOrigin = ui::pointerToNode(*cell.parent, event.position).x;
        state.gridResize = edge;
        state.gridMoved = false;
        if (edge != ui::NO_COLUMN)
            changePressed(state, nullptr);
        if (edge != ui::NO_COLUMN && event.clicks == 2)
            ui::setGridColumnWidth(*grid, edge, style::GridTrack{});
        else if (edge != ui::NO_COLUMN && edge < grid->content.grid->widths.size())
            state.gridGrab = grid->content.grid->widths[edge];
        if ((edge != ui::NO_COLUMN && event.clicks != 2) || (edge == ui::NO_COLUMN && grid->content.grid->view.reorderable))
            state.drag = &cell;
    }

    // Ширина — прежняя плюс сдвиг указателя: левый край столбца может уехать (доли соседей), а ручка не прыгает / The width is the former one plus the pointer shift: the column left edge may move (the neighbor shares), and the grip does not jump
    void dragGridHeader(RouterState& state, ui::Node& cell, Vec2 point)
    {
        ui::Node* grid = ui::gridOf(cell);

        if (grid == nullptr || cell.parent == nullptr)
            return;
        float x = ui::pointerToNode(*cell.parent, point).x;
        if (state.gridResize != ui::NO_COLUMN)
        {
            float width = state.gridGrab + x - state.gridOrigin;
            ui::setGridColumnWidth(*grid, state.gridResize, style::GridTrack{{style::TrackBoundKind::PIXELS, width},
                {style::TrackBoundKind::PIXELS, width}});
            return;
        }
        if (!state.gridMoved && std::fabs(x - state.gridOrigin) < GRID_DRAG_THRESHOLD)
            return;
        if (!state.gridMoved)
            changePressed(state, nullptr);
        state.gridMoved = true;
        reorderHeader(*grid, cell, x);
    }
}
