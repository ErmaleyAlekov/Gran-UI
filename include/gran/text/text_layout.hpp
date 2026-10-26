#ifndef GRAN_TEXT_TEXT_LAYOUT_HPP
#define GRAN_TEXT_TEXT_LAYOUT_HPP

#include "gran/text/font_library.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gran::text
{
    // Выравнивание строк / The line alignment
    enum class TextAlignment : std::uint8_t { START, CENTER, END, JUSTIFY };

    // Параметры текста (из вычисленного стиля) / The text parameters (from the computed style)
    struct TextStyle
    {
        std::string family;
        float size = 14.0f;
        int weight = 400;
        bool italic = false;
        float lineHeight = 1.3f;
        float letterSpacing = 0.0f;
        TextAlignment align = TextAlignment::START;
        bool wrap = true;
        bool ellipsis = false;
        int maxLines = 0;
    };

    // Глиф после формирования: начертание, номер, продвижение, байт символа в тексте, номер участка
    // A shaped glyph: the face, the number, the advance, the character byte in the text, the run number
    struct ShapedGlyph
    {
        const FontFace* face = nullptr;
        std::uint16_t glyph = 0;
        float advance = 0.0f;
        std::uint32_t cluster = 0;
        bool space = false;
        bool breakAfter = false;
        bool newline = false;
        std::uint16_t run = 0;
    };

    // Глиф на месте: x — от левого края блока, y — базовая линия строки, run — номер участка (TextRun)
    // A placed glyph: x from the box left edge, y is the line baseline, run is the run number (TextRun)
    struct PositionedGlyph
    {
        const FontFace* face = nullptr;
        std::uint16_t glyph = 0;
        float x = 0.0f;
        float y = 0.0f;
        std::uint32_t cluster = 0;
        std::uint16_t run = 0;
    };

    /*
    ** Участок оформленного текста: байты [begin, end) общего текста и свой стиль (шрифт, кегль,
    ** насыщенность, курсив, разрядка). Выравнивание, перенос и межстрочный интервал — общие, абзаца.
    **
    ** A styled text run: the bytes [begin, end) of the common text and its own style (the font, the size,
    ** the weight, italic, the spacing). The alignment, wrapping and line spacing are common, the paragraph's.
    */
    struct TextRun
    {
        std::uint32_t begin = 0;
        std::uint32_t end = 0;
        TextStyle style;
    };

    // Строка: глифы [first, first + count), ширина, базовая линия / A line: the glyphs [first, first + count), the width, the baseline
    struct TextLine
    {
        std::size_t first = 0;
        std::size_t count = 0;
        float width = 0.0f;
        float baseline = 0.0f;
    };

    // Место каретки перед символом с байтом cluster (и в конце строки) / The caret place before the character with byte cluster (and at the line end)
    struct CaretStop
    {
        std::uint32_t cluster = 0;
        float x = 0.0f;
        std::uint32_t line = 0;
    };

    /*
    ** Результат компоновки текста. faces удерживают начертания, на которые ссылаются глифы,
    ** поэтому раскладка живёт независимо от библиотеки шрифтов.
    **
    ** The text layout result. faces hold the faces the glyphs refer to,
    ** so the layout lives independently of the font library.
    */
    struct TextLayout
    {
        std::vector<PositionedGlyph> glyphs;
        std::vector<TextLine> lines;
        std::vector<CaretStop> carets;
        std::vector<std::shared_ptr<const FontFace>> faces;
        float fontSize = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    // ---- text_shaper.cpp ----

    // Глифы текста с заменой отсутствующих символов и кернингом / The text glyphs with missing character fallback and kerning
    std::vector<ShapedGlyph> shapeText(std::string_view text, const TextStyle& style, FontLibrary& fonts,
        std::vector<std::shared_ptr<const FontFace>>& faces);

    // ---- line_breaker.cpp ----

    // Границы строк [начало, конец) по ширине (бесконечность — только принудительные переносы)
    // The line bounds [begin, end) by width (infinity — only the forced breaks)
    std::vector<std::size_t> breakLines(const std::vector<ShapedGlyph>& glyphs, float maxWidth, bool wrap);

    // ---- text_runs.cpp ----

    /*
    ** Полная раскладка: формирование, перенос, многоточие, межстрочный интервал (half-leading
    ** как в CSS), выравнивание. maxWidth — ширина блока (бесконечность — без переноса).
    **
    ** The full layout: shaping, breaking, the ellipsis, the line spacing (half-leading
    ** as in CSS), the alignment. maxWidth is the box width (infinity — no wrapping).
    */
    TextLayout layoutText(std::string_view text, const TextStyle& style, float maxWidth, FontLibrary& fonts);

    /*
    ** Раскладка участками: каждый участок формируется своим стилем, глифы помечаются номером участка;
    ** перенос, выравнивание и интервал — по стилю абзаца style (как строки с <span> в HTML).
    ** Строки одной высоты: участок с большим кеглем межстрочный интервал не увеличивает.
    **
    ** A layout by runs: each run is shaped with its own style, the glyphs are marked with the run number;
    ** wrapping, alignment and spacing follow the paragraph style (as lines with <span> in HTML).
    ** The lines are of one height: a run with a larger size does not increase the line spacing.
    */
    TextLayout layoutRuns(std::string_view text, const std::vector<TextRun>& runs, const TextStyle& style, float maxWidth,
        FontLibrary& fonts);
}

#endif
