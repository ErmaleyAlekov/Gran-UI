#include "gran/layout/internal/layout_internal.hpp"

#include <atomic>
#include <cmath>

namespace gran::layout::internal
{
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

    // Рамка запоминается, дети размещаются в прямоугольнике содержимого; чистая ветвь того же размера — только сдвиг
    // The frame is remembered, the children are placed in the content rectangle; a clean branch of the same size is only shifted
    void arrangeNode(ui::Node& node, Rect frame, LayoutPass& pass)
    {
        if (reuseArrangement(node, frame))
            return;
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
