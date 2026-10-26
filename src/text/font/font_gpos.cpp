#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    namespace
    {
        // Размер ValueRecord: по 2 байта на каждый установленный бит формата / The ValueRecord size: 2 bytes per set format bit
        std::size_t valueSize(std::uint16_t format) noexcept
        {
            std::size_t size = 0;

            for (std::uint16_t bits = format; bits != 0; bits = static_cast<std::uint16_t>(bits >> 1))
                size += (bits & 1u) * 2;
            return size;
        }

        // Смещение XAdvance (бит 0x0004) в записи: после XPlacement и YPlacement, если они есть
        // The XAdvance offset (bit 0x0004) in the record: after XPlacement and YPlacement if present
        std::size_t advanceOffset(std::uint16_t format) noexcept
        {
            std::size_t offset = 0;

            if ((format & 0x0001) != 0)
                offset += 2;
            if ((format & 0x0002) != 0)
                offset += 2;
            return offset;
        }

        // Формат 1: набор пар для первого глифа, двоичный поиск второго / Format 1: the pair set of the first glyph, a binary search of the second
        bool pairFormat1(const FontReader& reader, std::size_t subtable, int covered, std::uint16_t right, float& value)
        {
            std::uint16_t format1 = reader.u16(subtable + 4);
            std::size_t record = 2 + valueSize(format1) + valueSize(reader.u16(subtable + 6));
            std::size_t low = 0;

            if (static_cast<std::size_t>(covered) >= reader.u16(subtable + 8))
                return false;
            std::size_t set = subtable + reader.u16(subtable + 10 + static_cast<std::size_t>(covered) * 2);
            std::size_t high = reader.u16(set);
            while (low < high)
            {
                std::size_t middle = (low + high) / 2;
                std::uint16_t second = reader.u16(set + 2 + middle * record);
                if (second == right)
                {
                    value = reader.i16(set + 2 + middle * record + 2 + advanceOffset(format1));
                    return (format1 & 0x0004) != 0;
                }
                if (second < right)
                    low = middle + 1;
                else
                    high = middle;
            }
            return false;
        }

        // Формат 2: классы обоих глифов → ячейка матрицы / Format 2: the classes of both glyphs → a matrix cell
        bool pairFormat2(const FontReader& reader, std::size_t subtable, std::uint16_t left, std::uint16_t right, float& value)
        {
            std::uint16_t format1 = reader.u16(subtable + 4);
            std::size_t record = valueSize(format1) + valueSize(reader.u16(subtable + 6));
            std::uint16_t first = glyphClass(reader, subtable + reader.u16(subtable + 8), left);
            std::uint16_t second = glyphClass(reader, subtable + reader.u16(subtable + 10), right);
            std::uint16_t columns = reader.u16(subtable + 14);

            if (first >= reader.u16(subtable + 12) || second >= columns || (format1 & 0x0004) == 0)
                return false;
            value = reader.i16(subtable + 16 + (static_cast<std::size_t>(first) * columns + second) * record
                + advanceOffset(format1));
            return true;
        }
    }

    // Левый глиф должен быть в покрытии подтаблицы / The left glyph must be in the subtable coverage
    bool pairAdjustment(const FontReader& reader, std::size_t subtable, std::uint16_t left, std::uint16_t right,
        float& value) noexcept
    {
        int covered = coverageIndex(reader, subtable + reader.u16(subtable + 2), left);

        if (covered < 0)
            return false;
        if (reader.u16(subtable) == 1)
            return pairFormat1(reader, subtable, covered, right, value);
        if (reader.u16(subtable) == 2)
            return pairFormat2(reader, subtable, left, right, value);
        return false;
    }
}
