#include "gran/text/internal/text_internal.hpp"

#include <cmath>

namespace gran::text
{
    namespace
    {
        // Состояние переноса: последняя возможность и ширина текущей строки / The break state: the last opportunity and the current line width
        struct BreakState
        {
            std::size_t lastBreak = 0;
            bool hasBreak = false;
            float width = 0.0f;
        };

        // Переполнение: разрыв после последней возможности или перед текущим символом (длинное слово)
        // An overflow: a break after the last opportunity or before the current character (a long word)
        void breakOverflow(const std::vector<ShapedGlyph>& glyphs, std::size_t index, BreakState& state,
            std::vector<std::size_t>& bounds)
        {
            std::size_t start = index;

            if (state.hasBreak)
                start = state.lastBreak + 1;
            bounds.push_back(start);
            state.width = 0.0f;
            for (std::size_t glyph = start; glyph < index; ++glyph)
                state.width += glyphs[glyph].advance;
            state.hasBreak = false;
        }
    }

    // Висячие пробелы и перевод строки в конце не входят в ширину (как в CSS)
    // Trailing hanging spaces and the line break are not part of the width (as in CSS)
    float lineWidth(const std::vector<ShapedGlyph>& glyphs, std::size_t first, std::size_t last) noexcept
    {
        float width = 0.0f;

        while (last > first && (glyphs[last - 1].space || glyphs[last - 1].newline))
            --last;
        for (std::size_t index = first; index < last; ++index)
            width += glyphs[index].advance;
        return width;
    }

    /*
    ** Жадный перенос: строка растёт, пока помещается; переполнение переносит её после
    ** последней возможности переноса, а слово длиннее строки режется по символам.
    ** Пробелы не вызывают переполнения (висят за краем). Перевод строки — всегда разрыв.
    **
    ** A greedy break: a line grows while it fits; an overflow breaks it after the
    ** last break opportunity, and a word longer than the line is cut by characters.
    ** Spaces do not cause an overflow (they hang beyond the edge). A line break always breaks.
    */
    std::vector<std::size_t> breakLines(const std::vector<ShapedGlyph>& glyphs, float maxWidth, bool wrap)
    {
        std::vector<std::size_t> bounds{0};
        BreakState state;
        bool limited = wrap && std::isfinite(maxWidth);

        for (std::size_t index = 0; index < glyphs.size(); ++index)
        {
            const ShapedGlyph& glyph = glyphs[index];
            bool visible = !glyph.space && !glyph.newline;
            if (limited && visible && index > bounds.back() && state.width + glyph.advance > maxWidth)
                breakOverflow(glyphs, index, state, bounds);
            state.width += glyph.advance;
            if (glyph.breakAfter)
            {
                state.lastBreak = index;
                state.hasBreak = true;
            }
            if (glyph.newline)
            {
                bounds.push_back(index + 1);
                state = BreakState{};
            }
        }
        bounds.push_back(glyphs.size());
        return bounds;
    }
}
