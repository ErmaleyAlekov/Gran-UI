#include "gran/layout/internal/layout_internal.hpp"
#include "gran/ui/data_grid.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gran::layout::internal
{
    namespace
    {
        /*
        ** Дорожка столбца. «1fr» в CSS Grid — minmax(auto, 1fr): доля не меньше содержимого. В таблице длинное
        ** значение должно обрезаться многоточием, а не раздвигать столбец, поэтому нижняя граница доли — minWidth.
        **
        ** The column track. "1fr" in CSS Grid is minmax(auto, 1fr): the share is not below the content. In a table a long
        ** value must be cut by an ellipsis instead of widening the column, so the lower bound of a share is minWidth.
        */
        style::GridTrack columnTrack(const ui::GridColumn& column) noexcept
        {
            style::GridTrack track = column.width;

            if (track.max.kind == style::TrackBoundKind::FRACTION && track.min.kind == style::TrackBoundKind::AUTO)
            {
                track.min.kind = style::TrackBoundKind::PIXELS;
                track.min.value = column.minWidth;
            }
            // Пределы входят в пиксельные границы до раздачи долей: иначе сумма столбцов вышла бы за таблицу
            // The limits enter the pixel bounds before the shares are dealt: otherwise the column sum would exceed the table
            if (track.min.kind == style::TrackBoundKind::PIXELS)
                track.min.value = clampSize(track.min.value, column.minWidth, column.maxWidth);
            if (track.max.kind == style::TrackBoundKind::PIXELS)
                track.max.value = clampSize(track.max.value, column.minWidth, column.maxWidth);
            return track;
        }

        // Столбцу нужна ширина содержимого: одна из границ — auto / The column needs the content width: one of the bounds is auto
        bool byContent(const style::GridTrack& track) noexcept
        {
            return track.min.kind == style::TrackBoundKind::AUTO || track.max.kind == style::TrackBoundKind::AUTO;
        }

        /*
        ** Наибольшее содержимое столбцов auto по заголовку и показанным строкам (у виртуальной таблицы — только
        ** окно строк). Максимум копится: при прокрутке столбец не сужается, и строки на экране не «прыгают».
        **
        ** The largest content of the auto columns by the header and the shown rows (for a virtual table — only
        ** the row window). The maximum accumulates: while scrolling a column does not narrow, and the rows on the screen do not "jump".
        */
        void measureColumns(ui::Node& grid, ui::GridState& state, LayoutPass& pass)
        {
            const float unbounded = std::numeric_limits<float>::infinity();
            std::vector<ui::Node*> lines{ui::gridHeader(grid)};
            ui::Node* rows = ui::gridRows(grid);

            state.autoWidths.resize(state.columns.size(), 0.0f);
            if (rows != nullptr)
            {
                for (const std::unique_ptr<ui::Node>& row : rows->children)
                    lines.push_back(row.get());
            }
            for (ui::Node* line : lines)
            {
                if (line == nullptr || !ui::inFlow(*line))
                    continue;
                for (std::size_t index = 0; index < line->children.size() && index < state.columns.size(); ++index)
                {
                    ui::Node& cell = *line->children[index];
                    if (!byContent(columnTrack(state.columns[index])) || !ui::inFlow(cell))
                        continue;
                    float width = measureNode(cell, Size{unbounded, unbounded}, pass).width + cell.computed.margin.left
                        + cell.computed.margin.right;
                    state.autoWidths[index] = std::max(state.autoWidths[index], width);
                }
            }
        }
    }

    /*
    ** Ширины столбцов — как дорожки сетки (sizeTracks): px — по значению, auto — по содержимому, fr делят остаток;
    ** пределы minWidth/maxWidth входят в дорожки и вклады заранее, после — только для долей. Без смены ширин строки не трогаются.
    **
    ** The column widths are like grid tracks (sizeTracks): px by value, auto by content, fr share the remainder;
    ** the minWidth/maxWidth limits enter the tracks and contributions beforehand, afterwards — only for shares. Without a width change the rows are left alone.
    */
    void resolveGridColumns(ui::Node& grid, float width, LayoutPass& pass)
    {
        ui::GridState* state = grid.content.grid.get();
        std::vector<style::GridTrack> tracks;
        std::vector<GridContribution> contributions;

        if (state == nullptr)
            return;
        measureColumns(grid, *state, pass);
        for (std::size_t index = 0; index < state->columns.size(); ++index)
        {
            tracks.push_back(columnTrack(state->columns[index]));
            if (byContent(tracks.back()))
                contributions.push_back(GridContribution{index, 1, clampSize(state->autoWidths[index],
                    state->columns[index].minWidth, state->columns[index].maxWidth)});
        }
        std::vector<float> widths = sizeTracks(tracks, contributions, width, 0.0f);
        for (std::size_t index = 0; index < widths.size(); ++index)
            widths[index] = clampSize(widths[index], state->columns[index].minWidth, state->columns[index].maxWidth);
        replaceGridWidths(grid, std::move(widths));
    }

    /*
    ** Таблица — прокручиваемый блок: строки прокручиваются внутри, поэтому высота содержимого не больше доступной
    ** (иначе родительский стек сжимал бы соседей таблицы, а внутри неё — строку заголовков).
    **
    ** A table is a scrolled box: the rows scroll inside, so the content height is not above the available one
    ** (otherwise the parent stack would shrink the table neighbors, and inside it — the header row).
    */
    Size measureDataGrid(ui::Node& grid, Size contentAvailable, LayoutPass& pass)
    {
        resolveGridColumns(grid, contentAvailable.width, pass);
        Size size = measureStack(grid, contentAvailable, pass);
        size.height = std::fmin(size.height, contentAvailable.height);
        return size;
    }
}
