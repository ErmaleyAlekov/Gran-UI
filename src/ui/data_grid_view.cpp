#include "gran/ui/data_grid.hpp"

#include <algorithm>

namespace gran::ui
{
    namespace
    {
        // Состояние таблицы строки (заголовок, копия списка или закреплённая строка); nullptr — не строка таблицы / The table state of a line (the header, a list copy or a pinned row); nullptr — not a table line
        const GridState* lineState(const Node& line) noexcept
        {
            Node* grid = nullptr;

            if (line.kind != ElementKind::GRID_ROW && line.kind != ElementKind::GRID_HEADER)
                return nullptr;
            grid = gridOf(line);
            if (grid == nullptr)
                return nullptr;
            return grid->content.grid.get();
        }
    }

    // Закреплённых не больше, чем ячеек в строке / The frozen ones are not more than the cells in the line
    std::size_t gridFrozen(const Node& line) noexcept
    {
        const GridState* state = lineState(line);

        if (state == nullptr)
            return 0;
        return std::min(state->view.frozen, line.children.size());
    }

    /*
    ** Закреплённая ячейка видна всегда, незакреплённая — если её столбец в окне прокрутки [shownFirst, shownEnd).
    ** До первой компоновки окно — «все столбцы» (shownEnd = NO_COLUMN).
    **
    ** A frozen cell is always shown, a non-frozen one — if its column is in the scroll viewport [shownFirst, shownEnd).
    ** Before the first layout the viewport is "all the columns" (shownEnd = NO_COLUMN).
    */
    bool gridCellShown(const Node& line, std::size_t index) noexcept
    {
        const GridState* state = lineState(line);

        if (state == nullptr || index < state->view.frozen)
            return true;
        return index >= state->view.shownFirst && index < state->view.shownEnd;
    }

    // Закреплённые ячейки размещены на своих местах (не сдвинуты прокруткой): край — правый край последней из них / The frozen cells are placed at their own places (not shifted by the scroll): the edge is the right edge of the last of them
    float gridFrozenEdge(const Node& line) noexcept
    {
        std::size_t frozen = gridFrozen(line);
        float edge = line.layout.frame.x;

        for (std::size_t index = 0; index < frozen; ++index)
        {
            const Node& cell = *line.children[index];
            if (inFlow(cell))
                edge = std::max(edge, cell.layout.frame.x + cell.layout.frame.width);
        }
        return edge;
    }

    // Видна и не под закреплёнными: незакреплённая ячейка левее их края скрыта отсечением (paintGridLine) / Shown and not under the frozen ones: a non-frozen cell left of their edge is hidden by the clip (paintGridLine)
    bool gridCellVisibleAt(const Node& line, std::size_t index, float x) noexcept
    {
        std::size_t frozen = gridFrozen(line);

        if (!gridCellShown(line, index))
            return false;
        return frozen == 0 || index < frozen || x >= gridFrozenEdge(line);
    }
}
