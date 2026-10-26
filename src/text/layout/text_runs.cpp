#include "gran/text/internal/text_internal.hpp"

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
                std::vector<ShapedGlyph> part = shapeText(text.substr(run.begin, run.end - run.begin), run.style, fonts,
                    runFaces);
                for (ShapedGlyph& glyph : part)
                {
                    glyph.cluster += run.begin;
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
        std::shared_ptr<const FontFace> primary = fonts.match(style.family, style.weight, style.italic);

        if (!primary)
            return layout;
        layout.faces.push_back(primary);
        std::vector<ShapedGlyph> glyphs = shapeRuns(text, runs, fonts, layout.faces);
        placeShaped(glyphs, style, maxWidth, layout);
        return layout;
    }
}
