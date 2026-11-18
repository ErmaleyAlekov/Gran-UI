#include "gran/ui/data_grid.hpp"

#include <algorithm>
#include <cmath>

namespace gran::ui
{
    namespace
    {
        // Сумма ширин первых count столбцов: левый край столбца count в содержимом прокрутки / The width sum of the first count columns: the left edge of the column count in the scroll content
        float columnStart(const std::vector<float>& widths, std::size_t count) noexcept
        {
            float start = 0.0f;

            for (std::size_t index = 0; index < count && index < widths.size(); ++index)
                start += widths[index];
            return start;
        }
    }

    /*
    ** Видимая полоса прокрутки — [смещение + закреплённые, смещение + окно). Столбец левее неё — смещение так,
    ** чтобы он встал сразу за закреплёнными; правее — чтобы его правый край совпал с краем окна; столбец шире
    ** полосы встаёт левым краем. Закреплённый текущий столбец виден всегда. Предел справа наложит компоновка.
    **
    ** The visible strip is [offset + frozen, offset + viewport). A column to its left — the offset so that it
    ** stands right after the frozen ones; to its right — so that its right edge meets the viewport edge; a column wider
    ** than the strip stands by its left edge. A frozen current column is always visible. The layout applies the right limit.
    */
    bool revealGridColumn(Node& grid)
    {
        GridState* state = grid.content.grid.get();
        Node* rows = gridRows(grid);

        if (state == nullptr || rows == nullptr || rows->parent == nullptr || state->current >= state->widths.size()
            || state->current < state->view.frozen)
            return false;
        Node& scroll = *rows->parent;
        const style::ComputedStyle& style = scroll.computed;
        float viewport = scroll.layout.frame.width - style.borderWidth.left - style.borderWidth.right
            - style.padding.left - style.padding.right;
        float frozen = columnStart(state->widths, state->view.frozen);
        float start = columnStart(state->widths, state->current) - frozen;
        float end = start + state->widths[state->current];
        float offset = scroll.layout.scrollOffset.x;
        float target = offset;
        if (end - target > viewport - frozen)
            target = end - (viewport - frozen);
        target = std::max(0.0f, std::min(target, start));
        if (std::fabs(target - offset) < 0.01f)
            return false;
        scroll.layout.scrollOffset.x = target;
        markDirty(scroll, DIRTY_LAYOUT | DIRTY_PAINT);
        return true;
    }
}
