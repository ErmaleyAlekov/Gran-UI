#include "gran/layout/internal/layout_internal.hpp"

#include <atomic>
#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Измерение из кэша текущего прохода / A measurement from the current pass cache
        bool findCached(const ui::Node& node, Size available, std::uint32_t generation, Size& result) noexcept
        {
            if (node.layout.generation != generation)
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

        // Два последних измерения: гибкий стек меряет ребёнка дважды (гипотеза и итог)
        // The two last measurements: a flexible stack measures a child twice (the hypothesis and the final)
        void storeCache(ui::Node& node, Size available, Size result, std::uint32_t generation) noexcept
        {
            if (node.layout.generation != generation)
            {
                node.layout.generation = generation;
                node.layout.cacheCount = 0;
            }
            std::uint8_t slot = node.layout.cacheCount;
            if (slot >= 2)
            {
                node.layout.cache[0] = node.layout.cache[1];
                slot = 1;
            }
            node.layout.cache[slot] = ui::MeasureEntry{available, result};
            node.layout.cacheCount = static_cast<std::uint8_t>(slot + 1);
        }
    }

    /*
    ** Размер рамки: явный размер или содержимое + padding + border, затем min/max.
    ** Содержимое меряется в пределах явного размера или доступного места и maxWidth/maxHeight.
    **
    ** The frame size: the explicit size or the content + padding + border, then min/max.
    ** The content is measured within the explicit size or the available space and maxWidth/maxHeight.
    */
    Size measureNode(ui::Node& node, Size available, LayoutPass& pass)
    {
        const style::ComputedStyle& style = node.computed;
        Insets insets = contentInsets(style);
        Size result{resolveLength(style.width, available.width), resolveLength(style.height, available.height)};
        Size inner = available;

        if (findCached(node, available, pass.generation, inner))
            return inner;
        if (!std::isnan(result.width))
            inner.width = result.width;
        if (!std::isnan(result.height))
            inner.height = result.height;
        inner.width = std::fmin(inner.width, style.maxWidth);
        inner.height = std::fmin(inner.height, style.maxHeight);
        Size content = measureContent(node, shrinkAvailable(inner, insets), pass);
        if (std::isnan(result.width))
            result.width = content.width + insets.left + insets.right;
        if (std::isnan(result.height))
            result.height = content.height + insets.top + insets.bottom;
        result.width = clampSize(result.width, style.minWidth, style.maxWidth);
        result.height = clampSize(result.height, style.minHeight, style.maxHeight);
        storeCache(node, available, result, pass.generation);
        return result;
    }

    // Рамка запоминается, дети размещаются в прямоугольнике содержимого
    // The frame is remembered, the children are placed in the content rectangle
    void arrangeNode(ui::Node& node, Rect frame, LayoutPass& pass)
    {
        node.layout.frame = frame;
        node.dirty = static_cast<std::uint8_t>((node.dirty & ~ui::DIRTY_LAYOUT) | ui::DIRTY_PAINT);
        arrangeContent(node, rectDeflate(frame, contentInsets(node.computed)), pass);
    }
}

namespace gran::layout
{
    /*
    ** Корень заполняет окно (за вычетом своих внешних отступов), если его размер не задан явно.
    ** Номер прохода — глобальный атомарный счётчик: деревья разных окон можно компоновать параллельно.
    **
    ** The root fills the window (minus its own margins) unless its size is set explicitly.
    ** The pass number is a global atomic counter: trees of different windows can be laid out in parallel.
    */
    void layoutTree(ui::Node& root, Size viewport, ContentMeasurer& measurer)
    {
        static std::atomic<std::uint32_t> counter{0};
        internal::LayoutPass pass{measurer, ++counter};
        Rect frame = rectDeflate(Rect{0.0f, 0.0f, viewport.width, viewport.height}, root.computed.margin);
        Size measured = internal::measureNode(root, Size{frame.width, frame.height}, pass);

        if (internal::hasFixedSize(root, true))
            frame.width = measured.width;
        if (internal::hasFixedSize(root, false))
            frame.height = measured.height;
        internal::arrangeNode(root, frame, pass);
        internal::layoutPopups(root, viewport, pass);
    }
}
