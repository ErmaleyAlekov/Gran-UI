#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>

namespace gran::layout::internal
{
    namespace
    {
        // Индекс и охват в пределах числа дорожек (схема это уже проверила; защита от правки дерева в коде)
        // The index and the span within the track count (the schema has checked it; a guard against tree edits in code)
        void clampCell(int index, int span, std::size_t count, std::size_t& first, std::size_t& covered) noexcept
        {
            first = std::min(static_cast<std::size_t>(std::max(0, index)), count - 1);
            covered = std::min(static_cast<std::size_t>(std::max(1, span)), count - first);
        }
    }

    // Видимые дети и их ячейки / The visible children and their cells
    void collectGridItems(ui::Node& grid, std::size_t rows, std::size_t columns, std::vector<GridItem>& items)
    {
        for (const std::unique_ptr<ui::Node>& child : grid.children)
        {
            if (!ui::inFlow(*child))
                continue;
            GridItem item;
            item.node = child.get();
            clampCell(child->computed.gridRow, child->computed.gridRowSpan, rows, item.row, item.rowSpan);
            clampCell(child->computed.gridColumn, child->computed.gridColumnSpan, columns, item.column,
                item.columnSpan);
            items.push_back(item);
        }
    }

    // Дорожка из списка или неявная auto / A track from the list or an implicit auto
    style::GridTrack trackAt(const std::vector<style::GridTrack>& tracks, std::size_t index) noexcept
    {
        if (index < tracks.size())
            return tracks[index];
        return style::GridTrack{};
    }

    // Пустой список — одна неявная дорожка auto / An empty list is one implicit auto track
    std::size_t trackCount(const std::vector<style::GridTrack>& tracks) noexcept
    {
        return std::max<std::size_t>(1, tracks.size());
    }

    // Размеры подряд плюс промежутки между ними / The sizes in a row plus the gaps between them
    float spanSize(const std::vector<float>& sizes, std::size_t first, std::size_t span, float gap) noexcept
    {
        float total = gap * static_cast<float>(span - 1);

        for (std::size_t index = first; index < first + span && index < sizes.size(); ++index)
            total += sizes[index];
        return total;
    }
}
