#include "gran/text/internal/text_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::text
{
    namespace
    {
        /*
        ** Предел строк и многоточие: лишние строки отбрасываются; последняя (или единственная
        ** без переноса) укорачивается, пока вместе с «…» не войдёт в ширину.
        **
        ** The line limit and the ellipsis: extra lines are dropped; the last one (or the single one
        ** without wrapping) is shortened until it fits the width together with "…".
        */
        void applyEllipsis(std::vector<ShapedGlyph>& glyphs, std::vector<std::size_t>& bounds, const TextStyle& style,
            float maxWidth, const FontFace& primary)
        {
            std::size_t lines = bounds.size() - 1;
            bool cut = style.maxLines > 0 && lines > static_cast<std::size_t>(style.maxLines);

            if (cut)
                bounds.resize(static_cast<std::size_t>(style.maxLines) + 1);
            std::size_t first = bounds[bounds.size() - 2];
            std::size_t last = bounds.back();
            bool overflow = std::isfinite(maxWidth) && lineWidth(glyphs, first, last) > maxWidth;
            if (!style.ellipsis || (!cut && !overflow))
                return;
            ShapedGlyph dots{&primary, primary.glyphIndex(0x2026), 0.0f, 0, false, false, false};
            dots.advance = primary.advance(dots.glyph) * style.size / primary.metrics().unitsPerEm;
            while (last > first && (glyphs[last - 1].space || lineWidth(glyphs, first, last) + dots.advance > maxWidth))
                --last;
            if (last < glyphs.size())
                dots.cluster = glyphs[last].cluster;
            glyphs.erase(glyphs.begin() + static_cast<std::ptrdiff_t>(last), glyphs.end());
            glyphs.push_back(dots);
            bounds.back() = glyphs.size();
        }

        // Пробелы строки — места растяжения при justify / The line spaces are the stretch points for justify
        std::size_t countSpaces(const std::vector<ShapedGlyph>& glyphs, const TextLine& line) noexcept
        {
            std::size_t spaces = 0;

            for (std::size_t index = line.first; index < line.first + line.count; ++index)
            {
                if (glyphs[index].space)
                    ++spaces;
            }
            return spaces;
        }

        // Смещение строки и добавка на пробел (justify) / The line offset and the addition per space (justify)
        float alignOffset(const TextStyle& style, float free, bool lastLine, std::size_t spaces, float& perSpace)
        {
            perSpace = 0.0f;
            if (!std::isfinite(free) || free <= 0.0f)
                return 0.0f;
            if (style.align == TextAlignment::CENTER)
                return std::round(free / 2.0f);
            if (style.align == TextAlignment::END)
                return free;
            if (style.align == TextAlignment::JUSTIFY && !lastLine && spaces > 0)
                perSpace = free / static_cast<float>(spaces);
            return 0.0f;
        }

        // Глифы одной строки на места и места каретки (пробелы невидимы, но каретка их обходит)
        // Placing the glyphs of one line and the caret places (spaces are invisible, but the caret steps over them)
        void placeLine(const std::vector<ShapedGlyph>& glyphs, const TextLine& line, float offset, float perSpace,
            TextLayout& layout)
        {
            float x = offset;
            std::uint32_t number = static_cast<std::uint32_t>(layout.lines.size());

            for (std::size_t index = line.first; index < line.first + line.count; ++index)
            {
                const ShapedGlyph& glyph = glyphs[index];
                layout.carets.push_back(CaretStop{glyph.cluster, x, number});
                if (!glyph.space && !glyph.newline)
                    layout.glyphs.push_back(PositionedGlyph{glyph.face, glyph.glyph, x, line.baseline, glyph.cluster, glyph.run});
                x += glyph.advance;
                if (glyph.space)
                    x += perSpace;
            }
            layout.carets.push_back(CaretStop{0xFFFFFFFFu, x, number});
        }
    }

    // Половинный интерлиньяж: строка высотой size × lineHeight, текст по центру её (как в CSS)
    // Half-leading: a line of height size × lineHeight with the text in its middle (as in CSS)
    void placeShaped(std::vector<ShapedGlyph>& glyphs, const TextStyle& style, float maxWidth, TextLayout& layout)
    {
        layout.fontSize = style.size;
        if (layout.faces.empty())
            return;
        const FontFace& primary = *layout.faces.front();
        std::vector<std::size_t> bounds = breakLines(glyphs, maxWidth, style.wrap);
        applyEllipsis(glyphs, bounds, style, maxWidth, primary);
        float scale = style.size / primary.metrics().unitsPerEm;
        float ascent = primary.metrics().ascender * scale;
        float lineHeight = style.size * style.lineHeight;
        float halfLeading = (lineHeight - (ascent - primary.metrics().descender * scale)) / 2.0f;
        for (std::size_t index = 0; index + 1 < bounds.size(); ++index)
        {
            TextLine line{bounds[index], bounds[index + 1] - bounds[index], lineWidth(glyphs, bounds[index], bounds[index + 1]),
                static_cast<float>(index) * lineHeight + halfLeading + ascent};
            float perSpace = 0.0f;
            float offset = alignOffset(style, maxWidth - line.width, index + 2 == bounds.size(),
                countSpaces(glyphs, line), perSpace);
            placeLine(glyphs, line, offset, perSpace, layout);
            layout.lines.push_back(line);
            layout.width = std::max(layout.width, line.width);
        }
        layout.height = static_cast<float>(layout.lines.size()) * lineHeight;
    }
}
