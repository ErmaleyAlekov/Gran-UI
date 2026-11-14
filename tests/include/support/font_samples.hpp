#ifndef GRAN_TESTS_SUPPORT_FONT_SAMPLES_HPP
#define GRAN_TESTS_SUPPORT_FONT_SAMPLES_HPP

#include <cstdint>
#include <string>
#include <vector>

/*
** Синтетические шрифты TrueType для тестов разбора испорченных глифов (замечания Т-41). Шрифт содержит
** только обязательные таблицы (head, hhea, maxp, hmtx, loca — длинная, glyf, cmap формата 4) и глифы
** ровно из переданных байтов: так тест сам решает, где глиф обрезан и куда ссылается компонент.
**
** Synthetic TrueType fonts for the damaged glyph parsing tests (remarks T-41). The font holds
** only the required tables (head, hhea, maxp, hmtx, loca — the long one, glyf, cmap format 4) and glyphs
** of exactly the given bytes: so a test decides by itself where a glyph is cut and where a component refers.
*/
namespace font_samples
{
    using Bytes = std::vector<std::uint8_t>;

    // Флаги записи компонента (OpenType glyf) / The component record flags (OpenType glyf)
    constexpr std::uint16_t ARGS_ARE_WORDS = 0x0001;
    constexpr std::uint16_t ARGS_ARE_XY = 0x0002;
    constexpr std::uint16_t MORE_COMPONENTS = 0x0020;
    constexpr std::uint16_t HAS_TWO_BY_TWO = 0x0080;

    // Файл шрифта: numGlyphs = glyphs.size(); байты глифов кладутся подряд, loca — по их длинам
    // The font file: numGlyphs = glyphs.size(); the glyph bytes go in a row, loca — by their lengths
    Bytes buildFont(const std::vector<Bytes>& glyphs);

    // Простой глиф: квадрат из 4 точек на кривой, один контур, без инструкций (34 байта)
    // A simple glyph: a square of 4 on-curve points, one contour, no instructions (34 bytes)
    Bytes squareGlyph();

    // Заголовок составного глифа (numberOfContours = −1, рамка нулевая; 10 байт)
    // The composite glyph header (numberOfContours = −1, a zero box; 10 bytes)
    Bytes compositeHeader();

    // Дописать запись компонента: флаги, глиф, смещение int8 (без ARGS_ARE_WORDS) или int16, без матрицы
    // Append a component record: the flags, the glyph, an int8 (without ARGS_ARE_WORDS) or int16 offset, no matrix
    void appendComponent(Bytes& glyph, std::uint16_t flags, std::uint16_t glyphIndex);

    // Записать байты во временный файл каталога тестов и вернуть путь / Write the bytes into a temporary test file and return the path
    std::string writeTemp(const Bytes& bytes, const std::string& name);
}

#endif
