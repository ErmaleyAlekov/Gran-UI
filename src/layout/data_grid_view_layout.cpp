#include "gran/layout/internal/layout_internal.hpp"
#include "gran/ui/data_grid.hpp"

#include <algorithm>
#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Сумма ширин первых count столбцов / The width sum of the first count columns
        float widthsBefore(const std::vector<float>& widths, std::size_t count) noexcept
        {
            float sum = 0.0f;

            for (std::size_t index = 0; index < count && index < widths.size(); ++index)
                sum += widths[index];
            return sum;
        }

        /*
        ** Незакреплённые столбцы, задевающие видимую полосу содержимого [смещение + закреплённые, смещение + окно):
        ** непрерывный отрезок [first, end). Видимых нет (закреплённые заняли всё окно) — пустой отрезок за концом.
        **
        ** The non-frozen columns touching the visible content strip [offset + frozen, offset + viewport):
        ** a contiguous range [first, end). None is visible (the frozen ones took the whole viewport) — an empty range past the end.
        */
        void shownRange(const ui::GridState& state, float offset, float viewport, std::size_t& first, std::size_t& end)
        {
            // Всё — в координатах содержимого прокрутки: столбец начинается в сумме ширин перед ним, видимая полоса —
            // [offset + закреплённые, offset + окно)
            // Everything is in the scroll content coordinates: a column starts at the width sum before it, the visible strip is
            // [offset + frozen, offset + viewport)
            std::size_t frozen = std::min(state.view.frozen, state.widths.size());
            float frozenWidth = widthsBefore(state.widths, frozen);
            float left = offset + frozenWidth;
            float right = offset + viewport;
            float x = frozenWidth;

            first = state.widths.size();
            end = first;
            for (std::size_t index = frozen; index < state.widths.size(); ++index)
            {
                float start = x;
                x += state.widths[index];
                if (x <= left || start >= right)
                    continue;
                if (first == state.widths.size())
                    first = index;
                end = index + 1;
            }
        }
    }

    /*
    ** Окно прокрутки — ширина таблицы без рамки и отступов самой прокрутки. Смещение ограничивается суммой столбцов
    ** (не шире, чем у arrangeScroll: содержимое прокрутки не уже столбцов, поэтому её предел не сдвинет наше смещение)
    ** и записывается в прокрутку — заголовок и строки встают по одному и тому же числу.
    **
    ** The scroll viewport is the table width without the scroll own border and padding. The offset is limited by the column sum
    ** (not wider than in arrangeScroll: the scroll content is not narrower than the columns, so its limit does not move our offset)
    ** and written into the scroll — the header and the rows stand by one and the same number.
    */
    void updateGridView(ui::Node& grid, float width)
    {
        ui::GridState& state = *grid.content.grid;
        ui::Node* rows = ui::gridRows(grid);
        std::size_t first = 0;
        std::size_t end = 0;

        if (rows == nullptr || rows->parent == nullptr)
            return;
        ui::Node& scroll = *rows->parent;
        Insets insets = contentInsets(scroll.computed);
        float viewport = std::fmax(0.0f, width - insets.left - insets.right);
        float total = widthsBefore(state.widths, state.widths.size());
        float offset = clampSize(scroll.layout.scrollOffset.x, 0.0f, std::fmax(0.0f, total - viewport));
        scroll.layout.scrollOffset.x = offset;
        shownRange(state, offset, viewport, first, end);
        bool changed = std::fabs(offset - state.view.scrollX) > 0.01f || first != state.view.shownFirst
            || end != state.view.shownEnd;
        state.view.scrollX = offset;
        state.view.shownFirst = first;
        state.view.shownEnd = end;
        if (changed)
            forgetGridLines(grid);
    }
}
