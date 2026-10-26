#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>

namespace gran::render::internal
{
    namespace
    {
        // Часть диапазона в строке: края x0..x1; continues — диапазон уходит за конец строки (перевод строки)
        // The range part in a line: the edges x0..x1; continues — the range goes past the line end (a line break)
        struct LineSpan
        {
            float x0 = 0.0f;
            float x1 = 0.0f;
            bool started = false;
            bool ended = false;
            bool continues = false;
        };

        /*
        ** Часть диапазона в строке line: x0 — первое место строки не левее from, x1 — последнее
        ** не правее to; continues — в диапазон входит перевод строки в её конце.
        **
        ** The range part in the line: x0 is the first line place not before from, x1 is the last one
        ** not after to; continues — the line break at its end is in the range.
        */
        LineSpan lineSpan(const text::TextLayout& layout, std::uint32_t line, const RangeFill& fill, const std::string& text)
        {
            LineSpan span;

            for (std::size_t index = 0; index < layout.carets.size(); ++index)
            {
                if (layout.carets[index].line != line)
                    continue;
                std::size_t offset = caretStopOffset(layout, index, text);
                if (!span.started && offset >= fill.from)
                {
                    span.x0 = layout.carets[index].x;
                    span.started = true;
                }
                if (offset <= fill.to)
                {
                    span.x1 = layout.carets[index].x;
                    span.ended = true;
                }
                span.continues = offset < fill.to && offset < text.size() && text[offset] == '\n';
            }
            return span;
        }
    }

    /*
    ** Прямоугольник под каждой строкой, которую задевает диапазон байтов показа [from, to).
    ** Полоса — от top на высоту height (0 — вся строка). Выделенный перевод строки (breaks) —
    ** полоска в треть кегля, чтобы было видно, что выделена и пустая строка.
    **
    ** A rectangle under each line the display byte range [from, to) touches.
    ** The strip goes from top for height (0 — the whole line). A selected line break (breaks) is
    ** a strip of a third of the font size, so that an empty selected line is visible too.
    */
    void paintRange(const TextPlace& place, const RangeFill& fill, const PaintState& state, SceneContext& context)
    {
        const ui::Node& node = place.node;
        std::string shown = displayedText(node);
        float lineHeight = node.computed.fontSize * node.computed.lineHeight;
        float height = fill.height;

        if (height <= 0.0f)
            height = lineHeight;
        for (std::uint32_t line = 0; fill.from < fill.to && line < place.layout.lines.size(); ++line)
        {
            LineSpan span = lineSpan(place.layout, line, fill, shown);
            if (fill.breaks && span.continues)
                span.x1 = std::max(span.x1, span.x0) + node.computed.fontSize * 0.3f;
            if (!span.started || !span.ended || span.x1 <= span.x0)
                continue;
            float unit = 1.0f;
            Rect rect{place.origin.x + span.x0, place.origin.y + static_cast<float>(line) * lineHeight + fill.top,
                span.x1 - span.x0, height};
            GpuInstance instance = placeInstance(state, rect, context.scale, unit);
            instance.kind = KIND_RECT;
            instance.fill = colorPackPremultiplied(fill.color);
            for (float& corner : instance.radii)
                corner = fill.radius * unit;
            context.scene.instances.push_back(instance);
        }
    }

    // Полупрозрачный accentColor под выделением (рисуется до глифов) / A translucent accentColor under the selection (painted before the glyphs)
    void paintSelection(const TextPlace& place, const PaintState& state, SceneContext& context)
    {
        const ui::NodeContent& content = place.node.content;
        RangeFill fill;

        // Раскладка — по показанному тексту (у пароля — точки), поэтому границы переводятся в байты показа
        // The layout is by the displayed text (dots for a password), so the bounds are turned into display bytes
        fill.from = toDisplayOffset(place.node, std::min(content.anchor, content.caret));
        fill.to = toDisplayOffset(place.node, std::max(content.anchor, content.caret));
        fill.color = place.node.computed.accentColor;
        fill.color.a *= 0.35f;
        fill.breaks = true;
        paintRange(place, fill, state, context);
    }
}
