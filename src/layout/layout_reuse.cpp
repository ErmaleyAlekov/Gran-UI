#include "gran/layout/internal/layout_internal.hpp"

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
            if (entry.available.width == available.width && entry.available.height == available.height)
            {
                result = entry.result;
                return true;
            }
        }
        return false;
    }

    /*
    ** Два последних измерения: гибкий стек меряет ребёнка дважды (гипотеза и итог). Грязный узел
    ** в новом проходе начинает кэш заново — прежние записи описывают старое содержимое.
    **
    ** The two last measurements: a flexible stack measures a child twice (the hypothesis and the final). A dirty node
    ** starts its cache anew in a new pass — the former entries describe the old content.
    */
    void storeCache(ui::Node& node, Size available, Size result, std::uint32_t generation) noexcept
    {
        if (node.layout.generation != generation && !clean(node))
            node.layout.cacheCount = 0;
        node.layout.generation = generation;
        std::uint8_t slot = node.layout.cacheCount;
        if (slot >= 2)
        {
            node.layout.cache[0] = node.layout.cache[1];
            slot = 1;
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
