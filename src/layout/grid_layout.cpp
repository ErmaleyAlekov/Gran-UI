#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <limits>

namespace gran::layout::internal
{
    namespace
    {
        // Размеры дорожек обеих осей / The track sizes of both axes
        struct GridSizes
        {
            std::vector<float> columns;
            std::vector<float> rows;
        };

        /*
        ** Вклады элементов: для столбцов — ширина по содержимому (без ограничения), для строк —
        ** высота при ширине своих столбцов. Размеры включают внешние отступы.
        **
        ** The item contributions: for columns — the content width (unbounded), for rows —
        ** the height at the width of their columns. The sizes include the margins.
        */
        std::vector<GridContribution> contributions(const std::vector<GridItem>& items, const ui::Node& grid,
            const std::vector<float>* columns, LayoutPass& pass)
        {
            const float unbounded = std::numeric_limits<float>::infinity();
            std::vector<GridContribution> result;

            for (const GridItem& item : items)
            {
                const Insets& margin = item.node->computed.margin;
                if (columns == nullptr)
                {
                    Size size = measureNode(*item.node, Size{unbounded, unbounded}, pass);
                    result.push_back(GridContribution{item.column, item.columnSpan, size.width + margin.left + margin.right});
                    continue;
                }
                float width = spanSize(*columns, item.column, item.columnSpan, grid.computed.columnSpacing)
                    - margin.left - margin.right;
                Size size = measureNode(*item.node, Size{std::max(0.0f, width), unbounded}, pass);
                result.push_back(GridContribution{item.row, item.rowSpan, size.height + margin.top + margin.bottom});
            }
            return result;
        }

        // Сначала столбцы, затем строки при известной ширине столбцов / Columns first, then rows at the known column widths
        GridSizes sizeGrid(ui::Node& grid, const std::vector<GridItem>& items, Size available, LayoutPass& pass)
        {
            const style::ComputedStyle& style = grid.computed;
            GridSizes sizes;

            sizes.columns = sizeTracks(style.columns, contributions(items, grid, nullptr, pass),
                available.width, style.columnSpacing);
            sizes.rows = sizeTracks(style.rows, contributions(items, grid, &sizes.columns, pass),
                available.height, style.rowSpacing);
            return sizes;
        }

        /*
        ** Элемент в ячейке: без явного размера растягивается на ячейку (минус внешние отступы),
        ** с явным — выравнивается по alignSelf (start по умолчанию) по обеим осям.
        **
        ** An item in a cell: without an explicit size it stretches over the cell (minus the margins),
        ** with one it is aligned by alignSelf (start by default) along both axes.
        */
        void placeGridItem(const GridItem& item, Rect cell, LayoutPass& pass)
        {
            Rect inner = rectDeflate(cell, item.node->computed.margin);
            Size measured = measureNode(*item.node, Size{inner.width, inner.height}, pass);
            Rect frame = inner;
            int align = item.node->computed.alignSelf;

            if (hasFixedSize(*item.node, true))
                frame.width = measured.width;
            if (hasFixedSize(*item.node, false))
                frame.height = measured.height;
            if (align == static_cast<int>(style::Align::CENTER))
            {
                frame.x += (inner.width - frame.width) / 2.0f;
                frame.y += (inner.height - frame.height) / 2.0f;
            }
            else if (align == static_cast<int>(style::Align::END))
            {
                frame.x += inner.width - frame.width;
                frame.y += inner.height - frame.height;
            }
            arrangeNode(*item.node, frame, pass);
        }
    }

    // Размер содержимого сетки — суммы дорожек с промежутками / The grid content size is the track sums with the gaps
    Size measureGrid(ui::Node& grid, Size contentAvailable, LayoutPass& pass)
    {
        std::vector<GridItem> items;

        collectGridItems(grid, trackCount(grid.computed.rows), trackCount(grid.computed.columns), items);
        GridSizes sizes = sizeGrid(grid, items, contentAvailable, pass);
        return Size{spanSize(sizes.columns, 0, sizes.columns.size(), grid.computed.columnSpacing),
            spanSize(sizes.rows, 0, sizes.rows.size(), grid.computed.rowSpacing)};
    }

    // Дорожки по месту, затем каждый элемент в свою ячейку / The tracks to fit, then every item into its cell
    void arrangeGrid(ui::Node& grid, Rect content, LayoutPass& pass)
    {
        const style::ComputedStyle& style = grid.computed;
        std::vector<GridItem> items;

        collectGridItems(grid, trackCount(style.rows), trackCount(style.columns), items);
        GridSizes sizes = sizeGrid(grid, items, Size{content.width, content.height}, pass);
        // Смещение дорожки k = сумма дорожек 0..k с промежутками минус сама дорожка k
        // The offset of the track k = the sum of the tracks 0..k with the gaps minus the track k itself
        for (const GridItem& item : items)
        {
            Rect cell{content.x + spanSize(sizes.columns, 0, item.column + 1, style.columnSpacing)
                    - sizes.columns[item.column],
                content.y + spanSize(sizes.rows, 0, item.row + 1, style.rowSpacing) - sizes.rows[item.row],
                spanSize(sizes.columns, item.column, item.columnSpan, style.columnSpacing),
                spanSize(sizes.rows, item.row, item.rowSpan, style.rowSpacing)};
            placeGridItem(item, cell, pass);
        }
    }
}
