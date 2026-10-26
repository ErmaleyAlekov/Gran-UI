#ifndef GRAN_TEXT_FONT_FACE_HPP
#define GRAN_TEXT_FONT_FACE_HPP

#include "gran/utils/geometry.hpp"
#include "gran/utils/mapped_file.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gran::text
{
    // Метрики начертания в единицах шрифта / The face metrics in font units
    struct FontMetrics
    {
        float unitsPerEm = 1000.0f;
        float ascender = 800.0f;
        float descender = -200.0f;
        float lineGap = 0.0f;
        float capHeight = 700.0f;
        float xHeight = 500.0f;
        int weight = 400;
        bool italic = false;
    };

    /*
    ** Контур глифа TrueType в единицах шрифта: точки с признаком «на кривой» и концы
    ** контуров. Две соседние точки вне кривой подразумевают точку на кривой посередине.
    **
    ** A TrueType glyph outline in font units: points with an "on curve" flag and the contour
    ** ends. Two adjacent off-curve points imply an on-curve point in the middle.
    */
    struct Outline
    {
        std::vector<Vec2> points;
        std::vector<std::uint8_t> onCurve;
        std::vector<std::uint16_t> contourEnds;
    };

    // Расположение таблиц внутри файла (смещение, длина) / The table locations inside the file (offset, length)
    struct TableSpan
    {
        std::uint32_t offset = 0;
        std::uint32_t length = 0;
    };

    // Таблицы, нужные движку / The tables the engine needs
    struct FaceTables
    {
        TableSpan head;
        TableSpan hhea;
        TableSpan maxp;
        TableSpan hmtx;
        TableSpan loca;
        TableSpan glyf;
        TableSpan cmap;
        TableSpan os2;
        TableSpan name;
        TableSpan kern;
        TableSpan gpos;
    };

    /*
    ** Начертание шрифта TrueType (glyf) из TTF или TTC. Данные шрифта — недоверенные:
    ** каждое чтение проверяет границы (FontReader), испорченный файл даёт GranException(FONT),
    ** а не чтение чужой памяти. Объект неизменяем после загрузки — его разделяют потоки.
    **
    ** A TrueType (glyf) font face from a TTF or TTC. The font data is untrusted:
    ** every read checks bounds (FontReader), a damaged file gives GranException(FONT),
    ** not a read of foreign memory. The object is immutable after loading — threads share it.
    */
    class FontFace
    {
    public:
        // ---- font_load.cpp ----

        /*
        ** Начертание с номером index (для TTC) из отображённого файла. Начертание держит файл
        ** (несколько начертаний TTC делят одно отображение) и читает его страницы по требованию.
        **
        ** The face number index (for TTC) from a mapped file. The face holds the file
        ** (several TTC faces share one mapping) and reads its pages on demand.
        */
        static std::shared_ptr<const FontFace> load(std::shared_ptr<const MappedFile> file, std::uint32_t index,
            const std::string& source);

        // Метрики / The metrics
        const FontMetrics& metrics() const noexcept;

        // Имя семейства и начертания из таблицы name / The family and subfamily names from the name table
        const std::string& family() const noexcept;
        const std::string& style() const noexcept;

        // ---- font_cmap.cpp ----

        // Глиф символа; 0 — символа нет в шрифте / The glyph of a character; 0 — the font has no such character
        std::uint16_t glyphIndex(char32_t symbol) const noexcept;

        // ---- font_metrics.cpp ----

        // Продвижение глифа в единицах шрифта / The glyph advance in font units
        float advance(std::uint16_t glyph) const noexcept;

        // Кернинг пары (kern или GPOS) в единицах шрифта / The pair kerning (kern or GPOS) in font units
        float kerning(std::uint16_t left, std::uint16_t right) const noexcept;

        // Число глифов / The glyph count
        std::uint16_t glyphCount() const noexcept;

        // ---- font_glyf.cpp ----

        // Контур глифа (составные раскрываются); false — пустой глиф (пробел) / The glyph outline (composites are expanded); false — an empty glyph (a space)
        bool outline(std::uint16_t glyph, Outline& result) const;

    private:
        FontFace() = default;

        // Внутренние части загрузки и разбора объявлены в internal/font_parts.hpp
        // The internal loading and parsing parts are declared in internal/font_parts.hpp
        friend struct FaceBuilder;

        std::shared_ptr<const MappedFile> _file;
        std::string _family;
        std::string _style;
        FontMetrics _metrics;
        FaceTables _tables;
        std::vector<std::uint32_t> _pairSubtables;
        std::uint32_t _cmapSubtable = 0;
        std::uint16_t _cmapFormat = 0;
        std::uint16_t _glyphCount = 0;
        std::uint16_t _horizontalMetrics = 0;
        bool _longLoca = false;
    };
}

#endif
