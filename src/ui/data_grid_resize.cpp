#include "gran/ui/data_grid.hpp"

#include <algorithm>
#include <cmath>

namespace gran::ui
{
    namespace
    {
        // Границы дорожки равны (вид и число с точностью до сотой пикселя) / The track bounds are equal (the kind and the number to a hundredth of a pixel)
        bool sameBound(const style::TrackBound& first, const style::TrackBound& second) noexcept
        {
            return first.kind == second.kind && std::fabs(first.value - second.value) <= 0.01f;
        }
    }

    /*
    ** Пиксельная ширина — в пределах столбца: не уже max(minWidth, GRID_MIN_RESIZE) (столбец не «теряется» под
    ** мышью), не шире maxWidth. Без std::clamp: при maxWidth меньше GRID_MIN_RESIZE нижняя граница выше верхней,
    ** и clamp дал бы неопределённое поведение; здесь побеждает maxWidth.
    **
    ** A pixel width stays within the column: not narrower than max(minWidth, GRID_MIN_RESIZE) (the column is not "lost" under
    ** the mouse), not wider than maxWidth. Without std::clamp: with maxWidth below GRID_MIN_RESIZE the lower bound is above the upper one,
    ** and clamp would give undefined behavior; here maxWidth wins.
    */
    float limitColumnWidth(const GridColumn& column, float width) noexcept
    {
        if (!(width >= 0.0f))
            width = 0.0f;
        width = std::max(width, std::max(column.minWidth, GRID_MIN_RESIZE));
        return std::min(width, column.maxWidth);
    }

    /*
    ** Ширина от пользователя: px (перетаскивание края, клавиши) или auto (двойной щелчок по краю — по содержимому).
    ** Столбец помнит, что ширину менял пользователь (resized), — она уходит в сохранённую раскладку; ширины
    ** пересчитает компоновка таблицы (флаг у таблицы).
    **
    ** A width from the user: px (dragging the edge, the keys) or auto (a double click on the edge — by the content).
    ** The column remembers the user changed the width (resized) — it goes into the saved layout; the table layout
    ** recomputes the widths (the flag on the table).
    */
    bool setGridColumnWidth(Node& grid, std::size_t column, const style::GridTrack& width)
    {
        GridState* state = grid.content.grid.get();
        style::GridTrack track = width;

        if (state == nullptr || column >= state->columns.size())
            return false;
        GridColumn& target = state->columns[column];
        if (track.min.kind == style::TrackBoundKind::PIXELS)
            track.min.value = limitColumnWidth(target, track.min.value);
        if (track.max.kind == style::TrackBoundKind::PIXELS)
            track.max.value = limitColumnWidth(target, track.max.value);
        if (sameBound(track.min, target.width.min) && sameBound(track.max, target.width.max))
            return false;
        target.width = track;
        target.resized = true;
        state->view.customized = true;
        markDirty(grid, DIRTY_LAYOUT | DIRTY_PAINT);
        return true;
    }
}
