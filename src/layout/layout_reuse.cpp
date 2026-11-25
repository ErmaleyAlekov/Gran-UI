#include "gran/layout/internal/layout_internal.hpp"

#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Узел не менялся с прошлой компоновки (флаг снимает arrangeNode) / The node has not changed since the last layout (arrangeNode clears the flag)
        bool clean(const ui::Node& node) noexcept
        {
            return (node.dirty & ui::DIRTY_LAYOUT) == 0;
        }

        // Сдвиг рамок всего поддерева: размеры и взаимное расположение сохраняются / Shifting the frames of the whole subtree: the sizes and the relative placement stay
        void translateTree(ui::Node& node, float dx, float dy) noexcept
        {
            node.layout.frame.x += dx;
            node.layout.frame.y += dy;
            node.dirty = static_cast<std::uint8_t>(node.dirty | ui::DIRTY_PAINT);
            for (const std::unique_ptr<ui::Node>& child : node.children)
                translateTree(*child, dx, dy);
        }
    }

    // Запись чистого узла живёт между проходами; у грязного — только в своём проходе
    // A clean node entry lives between passes; a dirty one's — only within its own pass
    bool findCached(const ui::Node& node, Size available, std::uint32_t generation, Size& result) noexcept
    {
        if (node.layout.generation != generation && !clean(node))
            return false;
        for (std::uint8_t index = 0; index < node.layout.cacheCount; ++index)
        {
            const ui::MeasureEntry& entry = node.layout.cache[index];
            const bool sameWidth = entry.available.width == available.width
                || (std::isnan(entry.available.width) && std::isnan(available.width));
            const bool sameHeight = entry.available.height == available.height
                || (std::isnan(entry.available.height) && std::isnan(available.height));
            if (sameWidth && sameHeight)
            {
                result = entry.result;
                return true;
            }
        }
        return false;
    }

    /*
    ** Последние MEASURE_CACHE измерений: гибкий стек меряет ребёнка дважды (гипотеза и итог), при сжатии — ещё
    ** и при нулевой высоте (автоминимум). Грязный узел
    ** в новом проходе начинает кэш заново — прежние записи описывают старое содержимое.
    **
    ** The last MEASURE_CACHE measurements: a flexible stack measures a child twice (the hypothesis and the final), when shrinking
    ** also at a zero height (the automatic minimum). A dirty node
    ** starts its cache anew in a new pass — the former entries describe the old content.
    */
    void storeCache(ui::Node& node, Size available, Size result, std::uint32_t generation) noexcept
    {
        if (node.layout.generation != generation && !clean(node))
            node.layout.cacheCount = 0;
        node.layout.generation = generation;
        std::uint8_t slot = node.layout.cacheCount;
        // Полный кэш: самая старая запись уходит, остальные сдвигаются / A full cache: the oldest entry goes, the others shift
        if (slot >= ui::MEASURE_CACHE)
        {
            for (std::size_t index = 1; index < ui::MEASURE_CACHE; ++index)
                node.layout.cache[index - 1] = node.layout.cache[index];
            slot = static_cast<std::uint8_t>(ui::MEASURE_CACHE - 1);
        }
        node.layout.cache[slot] = ui::MeasureEntry{available, result};
        node.layout.cacheCount = static_cast<std::uint8_t>(slot + 1);
    }

    // Размер сравнивается точно: те же числа на входе дают те же числа на выходе / The size is compared exactly: the same input numbers give the same output numbers
    bool reuseArrangement(ui::Node& node, Rect frame) noexcept
    {
        const Rect& previous = node.layout.frame;

        if (!clean(node) || previous.width != frame.width || previous.height != frame.height)
            return false;
        if (previous.x != frame.x || previous.y != frame.y)
            translateTree(node, frame.x - previous.x, frame.y - previous.y);
        return true;
    }
}
