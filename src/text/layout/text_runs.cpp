#include "gran/text/internal/text_internal.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/utils/utf8.hpp"

#include <algorithm>

namespace gran::text
{
    namespace
    {
        // Начертания участка добавляются к общим без повторов (основное абзаца остаётся первым)
        // The run faces are added to the common ones without repeats (the paragraph primary stays first)
        void mergeFaces(std::vector<std::shared_ptr<const FontFace>>& faces,
            const std::vector<std::shared_ptr<const FontFace>>& added)
        {
            for (const std::shared_ptr<const FontFace>& face : added)
            {
                if (std::find(faces.begin(), faces.end(), face) == faces.end())
                    faces.push_back(face);
            }
        }

        /*
        ** Каждый участок формируется своим стилем; номера байтов сдвигаются на начало участка,
        ** глиф помечается номером участка (по нему рендерер берёт цвет и размер). Кернинг
        ** через границу участков не применяется: соседние участки — разные начертания.
        **
        ** Each run is shaped with its own style; the byte numbers are shifted by the run start,
        ** the glyph is marked with the run number (the renderer takes the color and the size by it). Kerning
        ** across a run boundary is not applied: adjacent runs are different faces.
        */
        std::vector<ShapedGlyph> shapeRuns(std::string_view text, const std::vector<TextRun>& runs, FontLibrary& fonts,
            std::vector<std::shared_ptr<const FontFace>>& faces)
        {
            std::vector<ShapedGlyph> glyphs;

            for (std::size_t index = 0; index < runs.size(); ++index)
            {
                const TextRun& run = runs[index];
                std::vector<std::shared_ptr<const FontFace>> runFaces;
                if (run.begin >= run.end || run.end > text.size())
                    continue;
                auto begin = run.begin;
                if (begin > 0 && text[begin] == '\n' && text[begin - 1] == '\r' && !glyphs.empty() &&
                    glyphs.back().newline && glyphs.back().cluster == begin - 1)
                    ++begin;
                if (begin == run.end)
                    continue;
                std::vector<ShapedGlyph> part = shapeText(text.substr(begin, run.end - begin), run.style, fonts,
                    runFaces);
                if (!part.empty() && !glyphs.empty())
                {
                    const auto& previous = runs[glyphs.back().run];
                    std::size_t before = glyphs.back().cluster - previous.begin;
                    std::size_t after = part.front().cluster;
                    glyphs.back().breakAfter = breakOpportunityAfter(
                        decodeUtf8(text.substr(previous.begin, previous.end - previous.begin), before),
                        decodeUtf8(text.substr(begin, run.end - begin), after));
                }
                for (ShapedGlyph& glyph : part)
                {
                    glyph.cluster += begin;
                    glyph.run = static_cast<std::uint16_t>(index);
                    glyphs.push_back(glyph);
                }
                mergeFaces(faces, runFaces);
            }
            return glyphs;
        }
    }

    // Один стиль на весь текст / One style for the whole text
    TextLayout layoutText(std::string_view text, const TextStyle& style, float maxWidth, FontLibrary& fonts)
    {
        TextLayout layout;
        std::vector<ShapedGlyph> glyphs = shapeText(text, style, fonts, layout.faces);

        placeShaped(glyphs, style, maxWidth, layout);
        return layout;
    }

    // Основное начертание абзаца — первым: от него межстрочный интервал и многоточие / The paragraph primary face goes first: the line spacing and the ellipsis come from it
    TextLayout layoutRuns(std::string_view text, const std::vector<TextRun>& runs, const TextStyle& style, float maxWidth,
        FontLibrary& fonts)
    {
        TextLayout layout;
        if (runs.size() > UINT16_MAX || text.size() > UINT32_MAX)
            throw GranException(ErrorCode::LIMIT_EXCEEDED, i18n::format(i18n::Message::TEXT_LAYOUT_LIMIT));
        std::shared_ptr<const FontFace> primary = fonts.match(style.family, style.weight, style.italic);

        if (!primary)
            return layout;
        layout.faces.push_back(primary);
        std::vector<ShapedGlyph> glyphs = shapeRuns(text, runs, fonts, layout.faces);
        placeShaped(glyphs, style, maxWidth, layout);
        return layout;
    }
}
