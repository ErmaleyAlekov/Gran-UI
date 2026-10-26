#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    namespace
    {
        // Двоичный поиск записи диапазона [start, end] (записи по size байт от first) / A binary search of a [start, end] range record
        bool findRange(const FontReader& reader, std::size_t first, std::size_t count, std::size_t size,
            std::uint16_t glyph, std::size_t& record) noexcept
        {
            std::size_t low = 0;
            std::size_t high = count;

            while (low < high)
            {
                std::size_t middle = (low + high) / 2;
                std::size_t entry = first + middle * size;
                if (reader.u16(entry + 2) < glyph)
                    low = middle + 1;
                else if (reader.u16(entry) > glyph)
                    high = middle;
                else
                {
                    record = entry;
                    return true;
                }
            }
            return false;
        }
    }

    // Тег из четырёх символов строки / A tag from the four string characters
    std::uint32_t fontTag(const char* name) noexcept
    {
        return (static_cast<std::uint32_t>(static_cast<unsigned char>(name[0])) << 24)
            | (static_cast<std::uint32_t>(static_cast<unsigned char>(name[1])) << 16)
            | (static_cast<std::uint32_t>(static_cast<unsigned char>(name[2])) << 8)
            | static_cast<std::uint32_t>(static_cast<unsigned char>(name[3]));
    }

    // Формат 1 — упорядоченный список глифов, формат 2 — диапазоны / Format 1 — an ordered glyph list, format 2 — ranges
    int coverageIndex(const FontReader& reader, std::size_t coverage, std::uint16_t glyph) noexcept
    {
        std::size_t count = reader.u16(coverage + 2);
        std::size_t record = 0;

        if (reader.u16(coverage) == 2)
        {
            if (!findRange(reader, coverage + 4, count, 6, glyph, record))
                return -1;
            return reader.u16(record + 4) + (glyph - reader.u16(record));
        }
        std::size_t low = 0;
        std::size_t high = count;
        while (low < high)
        {
            std::size_t middle = (low + high) / 2;
            std::uint16_t value = reader.u16(coverage + 4 + middle * 2);
            if (value == glyph)
                return static_cast<int>(middle);
            if (value < glyph)
                low = middle + 1;
            else
                high = middle;
        }
        return -1;
    }

    // Формат 1 — массив классов от startGlyph, формат 2 — диапазоны; вне описания — класс 0
    // Format 1 — a class array from startGlyph, format 2 — ranges; outside the description — class 0
    std::uint16_t glyphClass(const FontReader& reader, std::size_t classDef, std::uint16_t glyph) noexcept
    {
        std::size_t record = 0;

        if (reader.u16(classDef) == 2)
        {
            if (!findRange(reader, classDef + 4, reader.u16(classDef + 2), 6, glyph, record))
                return 0;
            return reader.u16(record + 4);
        }
        std::uint16_t start = reader.u16(classDef + 2);
        if (glyph < start || glyph - start >= reader.u16(classDef + 4))
            return 0;
        return reader.u16(classDef + 6 + static_cast<std::size_t>(glyph - start) * 2);
    }
}
