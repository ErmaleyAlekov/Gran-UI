#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    /*
    ** Формат 4: сегменты [startCode, endCode] упорядочены, поиск двоичный по endCode.
    ** Глиф: при idRangeOffset = 0 — (символ + idDelta) mod 65536, иначе — из glyphIdArray
    ** по адресу относительно самого поля idRangeOffset (особенность формата).
    **
    ** Format 4: the [startCode, endCode] segments are ordered, the search is binary by endCode.
    ** The glyph: with idRangeOffset = 0 — (character + idDelta) mod 65536, otherwise from glyphIdArray
    ** at an address relative to the idRangeOffset field itself (a format quirk).
    */
    std::uint16_t cmapFormat4(const FontReader& reader, std::uint32_t subtable, char32_t symbol) noexcept
    {
        std::size_t segments = reader.u16(subtable + 6) / 2;
        std::size_t ends = subtable + 14;
        std::size_t low = 0;
        std::size_t high = segments;

        if (symbol > 0xFFFF)
            return 0;
        while (low < high)
        {
            std::size_t middle = (low + high) / 2;
            if (reader.u16(ends + middle * 2) < symbol)
                low = middle + 1;
            else
                high = middle;
        }
        std::size_t starts = ends + segments * 2 + 2;
        if (low >= segments || reader.u16(starts + low * 2) > symbol)
            return 0;
        std::uint16_t delta = reader.u16(starts + segments * 2 + low * 2);
        std::size_t rangeField = starts + segments * 4 + low * 2;
        std::uint16_t range = reader.u16(rangeField);
        if (range == 0)
            return static_cast<std::uint16_t>(symbol + delta);
        std::uint16_t glyph = reader.u16(rangeField + range + (symbol - reader.u16(starts + low * 2)) * 2);
        if (glyph == 0)
            return 0;
        return static_cast<std::uint16_t>(glyph + delta);
    }

    // Формат 12: группы [start, end] → startGlyph + (символ − start), поиск двоичный
    // Format 12: the groups [start, end] → startGlyph + (character − start), a binary search
    std::uint16_t cmapFormat12(const FontReader& reader, std::uint32_t subtable, char32_t symbol) noexcept
    {
        std::size_t low = 0;
        std::size_t high = reader.u32(subtable + 12);

        while (low < high)
        {
            std::size_t middle = (low + high) / 2;
            std::size_t group = subtable + 16 + middle * 12;
            if (reader.u32(group + 4) < symbol)
                low = middle + 1;
            else if (reader.u32(group) > symbol)
                high = middle;
            else
                return static_cast<std::uint16_t>(reader.u32(group + 8) + (symbol - reader.u32(group)));
        }
        return 0;
    }

    // Подтаблица выбрана при загрузке / The subtable is chosen at load time
    std::uint16_t FontFace::glyphIndex(char32_t symbol) const noexcept
    {
        FontReader reader(_file->data(), _file->size());
        std::uint16_t glyph = 0;

        if (_cmapFormat == 12)
            glyph = cmapFormat12(reader, _cmapSubtable, symbol);
        else
            glyph = cmapFormat4(reader, _cmapSubtable, symbol);
        if (glyph >= _glyphCount)
            return 0;
        return glyph;
    }
}
