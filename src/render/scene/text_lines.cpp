#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    // Обычное место, конец последней строки или конец строки перед переводом строки — пригодны
    // A plain place, the end of the last line or a line end before a line break are usable
    bool usableStop(const text::TextLayout& layout, std::size_t index, const std::string& shown) noexcept
    {
        if (layout.carets[index].cluster != 0xFFFFFFFFu || index + 1 == layout.carets.size())
            return true;
        std::size_t offset = caretStopOffset(layout, index, shown);
        return offset < shown.size() && shown[offset] == '\n';
    }

    // Перебор мест строки с наименьшим расстоянием по x / Walking the line places for the least x distance
    std::size_t nearestStop(const text::TextLayout& layout, std::uint32_t line, float x, const std::string& shown) noexcept
    {
        std::size_t best = layout.carets.size();
        float bestDistance = 0.0f;

        for (std::size_t index = 0; index < layout.carets.size(); ++index)
        {
            float distance = std::fabs(layout.carets[index].x - x);
            if (layout.carets[index].line != line || !usableStop(layout, index, shown))
                continue;
            if (best != layout.carets.size() && distance >= bestDistance)
                continue;
            best = index;
            bestDistance = distance;
        }
        return best;
    }
}

namespace gran::render
{
    /*
    ** Край строки каретки: первое (end = false) или последнее пригодное место строки.
    ** Конец строки с мягким переносом — перед её последним символом (обычно висячим пробелом).
    **
    ** The caret line edge: the first (end = false) or the last usable place of the line.
    ** The end of a soft-wrapped line is before its last character (usually a hanging space).
    */
    std::size_t textLineEdge(TextEngine& engine, const ui::Node& node, std::size_t offset, bool end)
    {
        Rect box = internal::contentRect(node);
        const text::TextLayout& layout = engine.layoutFor(node, box.width);
        std::string shown = displayedText(node);
        std::size_t edge = layout.carets.size();

        if (node.content.text.empty())
            return 0;
        text::CaretStop caret = internal::caretStopAt(layout, toDisplayOffset(node, offset), shown);
        for (std::size_t index = 0; index < layout.carets.size(); ++index)
        {
            if (layout.carets[index].line != caret.line || !internal::usableStop(layout, index, shown))
                continue;
            if (end || edge == layout.carets.size())
                edge = index;
        }
        if (edge == layout.carets.size())
            return offset;
        return toContentOffset(node, internal::caretStopOffset(layout, edge, shown));
    }

    /*
    ** Переход на lines строк: место — ближайшее к goalX (NaN — к x каретки; на выходе — использованный x).
    ** Выше первой строки — начало текста, ниже последней — конец (как в полях браузеров).
    **
    ** Moving by lines: the place nearest to goalX (NaN — to the caret x; on output — the x used).
    ** Above the first line is the text start, below the last one — the end (as in browser fields).
    */
    std::size_t textLineMove(TextEngine& engine, const ui::Node& node, std::size_t offset, int lines, float& goalX)
    {
        Rect box = internal::contentRect(node);
        const text::TextLayout& layout = engine.layoutFor(node, box.width);
        std::string shown = displayedText(node);

        if (node.content.text.empty())
            return 0;
        text::CaretStop caret = internal::caretStopAt(layout, toDisplayOffset(node, offset), shown);
        long target = static_cast<long>(caret.line) + lines;
        if (std::isnan(goalX))
            goalX = caret.x;
        if (target < 0)
            return 0;
        if (target >= static_cast<long>(layout.lines.size()))
            return node.content.text.size();
        std::size_t stop = internal::nearestStop(layout, static_cast<std::uint32_t>(target), goalX, shown);
        if (stop == layout.carets.size())
            return offset;
        return toContentOffset(node, internal::caretStopOffset(layout, stop, shown));
    }

    // Целые строки в прямоугольнике содержимого, но не меньше одной / Whole lines in the content rectangle, but at least one
    int textPageLines(const ui::Node& node) noexcept
    {
        float lineHeight = node.computed.fontSize * node.computed.lineHeight;

        if (lineHeight <= 0.0f)
            return 1;
        return std::max(1, static_cast<int>(std::floor(internal::contentRect(node).height / lineHeight)));
    }
}
