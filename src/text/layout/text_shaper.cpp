#include "gran/text/internal/text_internal.hpp"
#include "gran/utils/utf8.hpp"

#include <algorithm>

namespace gran::text
{
    namespace
    {
        // Символ и номер его первого байта в тексте / A character and the number of its first byte in the text
        struct Symbol
        {
            char32_t value;
            std::uint32_t cluster;
        };

        // Библиотека и удерживаемые начертания (первое — основное) / The library and the held faces (the first is the primary)
        struct ShapeContext
        {
            FontLibrary& fonts;
            std::vector<std::shared_ptr<const FontFace>>& faces;
        };

        // Все символы с номерами байтов; \r пропускается (\r\n — один перевод строки)
        // All characters with byte numbers; \r is skipped (\r\n is one line break)
        std::vector<Symbol> decodeAll(std::string_view text)
        {
            std::vector<Symbol> symbols;

            for (std::size_t position = 0; position < text.size();)
            {
                std::uint32_t cluster = static_cast<std::uint32_t>(position);
                char32_t value = decodeUtf8(text, position);
                if (value != '\r')
                    symbols.push_back(Symbol{value, cluster});
            }
            return symbols;
        }

        // Начертание с глифом символа: основное или замена (удерживается в faces) / The face with the character glyph: the primary or a fallback (held in faces)
        const FontFace* selectFace(char32_t symbol, const TextStyle& style, ShapeContext& context, std::uint16_t& glyph)
        {
            const FontFace* primary = context.faces.front().get();

            glyph = primary->glyphIndex(symbol);
            if (glyph != 0 || symbol < 0x20)
                return primary;
            std::shared_ptr<const FontFace> fallback = context.fonts.fallback(symbol, style.weight, style.italic);
            if (!fallback)
                return primary;
            if (std::find(context.faces.begin(), context.faces.end(), fallback) == context.faces.end())
                context.faces.push_back(fallback);
            glyph = fallback->glyphIndex(symbol);
            return fallback.get();
        }

        // Один символ: начертание, глиф, продвижение (табуляция — четыре пробела), признаки переноса
        // One character: the face, the glyph, the advance (a tab is four spaces), the break flags
        ShapedGlyph shapeSymbol(const Symbol& symbol, char32_t next, const TextStyle& style, ShapeContext& context)
        {
            ShapedGlyph shaped;
            char32_t visible = symbol.value;
            float repeat = 1.0f;

            if (symbol.value == '\t')
            {
                visible = ' ';
                repeat = 4.0f;
            }
            shaped.cluster = symbol.cluster;
            shaped.newline = symbol.value == '\n';
            shaped.space = isBreakingSpace(symbol.value);
            shaped.breakAfter = breakOpportunityAfter(symbol.value, next);
            shaped.face = selectFace(visible, style, context, shaped.glyph);
            float scale = style.size / shaped.face->metrics().unitsPerEm;
            shaped.advance = shaped.face->advance(shaped.glyph) * scale * repeat + style.letterSpacing;
            if (shaped.newline)
                shaped.advance = 0.0f;
            return shaped;
        }
    }

    // Кернинг прибавляется к предыдущему глифу того же начертания / The kerning is added to the previous glyph of the same face
    std::vector<ShapedGlyph> shapeText(std::string_view text, const TextStyle& style, FontLibrary& fonts,
        std::vector<std::shared_ptr<const FontFace>>& faces)
    {
        std::vector<Symbol> symbols = decodeAll(text);
        std::vector<ShapedGlyph> glyphs;
        std::shared_ptr<const FontFace> primary = fonts.match(style.family, style.weight, style.italic);
        ShapeContext context{fonts, faces};

        if (!primary)
            return glyphs;
        faces.insert(faces.begin(), primary);
        for (std::size_t index = 0; index < symbols.size(); ++index)
        {
            char32_t next = 0;
            if (index + 1 < symbols.size())
                next = symbols[index + 1].value;
            ShapedGlyph shaped = shapeSymbol(symbols[index], next, style, context);
            if (!glyphs.empty() && glyphs.back().face == shaped.face)
                glyphs.back().advance += shaped.face->kerning(glyphs.back().glyph, shaped.glyph)
                    * style.size / shaped.face->metrics().unitsPerEm;
            glyphs.push_back(shaped);
        }
        return glyphs;
    }
}
