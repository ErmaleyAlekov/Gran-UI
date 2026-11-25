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
        PairResult pairFormat1(const FontReader& reader, std::size_t subtable, int covered, std::uint16_t right, float& value)
        {
            std::uint16_t format1 = reader.u16(subtable + 4);
            std::size_t record = 2 + valueSize(format1) + valueSize(reader.u16(subtable + 6));
            std::size_t low = 0;

            const std::size_t count = reader.u16(subtable + 8);
            if (static_cast<std::size_t>(covered) >= count ||
                !reader.contains(subtable + 10, count * 2))
                return PairResult::DAMAGED;
            std::size_t set = subtable + reader.u16(subtable + 10 + static_cast<std::size_t>(covered) * 2);
            std::size_t high = reader.u16(set);
            if (set < subtable + 10 + count * 2 || !reader.contains(set, 2) || !reader.contains(set + 2, high * record))
                return PairResult::DAMAGED;
            if ((format1 & 4) == 0)
                return PairResult::ABSENT;
            while (low < high)
            {
                std::size_t middle = (low + high) / 2;
                std::uint16_t second = reader.u16(set + 2 + middle * record);
                if (second == right)
                {
                    value = reader.i16(set + 2 + middle * record + 2 + advanceOffset(format1));
                    return PairResult::MATCH;
                }
                if (second < right)
                    low = middle + 1;
                else
                    high = middle;
            }
            return PairResult::ABSENT;
        }

        // Формат 2: классы обоих глифов → ячейка матрицы / Format 2: the classes of both glyphs → a matrix cell
        PairResult pairFormat2(const FontReader& reader, std::size_t subtable, std::uint16_t left, std::uint16_t right, float& value)
        {
            std::uint16_t format1 = reader.u16(subtable + 4);
            std::size_t record = valueSize(format1) + valueSize(reader.u16(subtable + 6));
            int first = glyphClass(reader, subtable + reader.u16(subtable + 8), left);
            int second = glyphClass(reader, subtable + reader.u16(subtable + 10), right);
            std::uint16_t columns = reader.u16(subtable + 14);
            const auto rows = reader.u16(subtable + 12);
            const std::size_t rowSize = columns * record;

            if (first < 0 || second < 0 || first >= rows || second >= columns ||
                (rowSize && rows > SIZE_MAX / rowSize) || !reader.contains(subtable + 16, rows * rowSize))
                return PairResult::DAMAGED;
            const auto matrixEnd = 16 + rows * rowSize;
            if (reader.u16(subtable + 2) < matrixEnd || reader.u16(subtable + 8) < matrixEnd ||
                reader.u16(subtable + 10) < matrixEnd)
                return PairResult::DAMAGED;
            if ((format1 & 4) == 0)
                return PairResult::ABSENT;
            value = reader.i16(subtable + 16 + (static_cast<std::size_t>(first) * columns + second) * record
                + advanceOffset(format1));
            return PairResult::MATCH;
        }
    }

    // Левый глиф должен быть в покрытии подтаблицы / The left glyph must be in the subtable coverage
    PairResult pairAdjustment(const FontReader& reader, std::size_t subtable, std::uint16_t left, std::uint16_t right,
        float& value) noexcept
    {
        const auto table = reader.from(subtable);
        if (!table.contains(0, 10) || (table.u16(0) != 1 && table.u16(0) != 2) ||
            (table.u16(4) | table.u16(6)) > 255)
            return PairResult::DAMAGED;
        const std::size_t header = table.u16(0) == 1 ? 10u + 2u * table.u16(8) : 16u;
        if (!table.contains(0, header) || table.u16(2) < header)
            return PairResult::DAMAGED;
        int covered = coverageIndex(table, table.u16(2), left);

        if (covered < 0)
            return covered == -1 ? PairResult::ABSENT : PairResult::DAMAGED;
        if (table.u16(0) == 1)
            return pairFormat1(table, 0, covered, right, value);
        if (table.u16(8) >= header && table.u16(10) >= header)
            return pairFormat2(table, 0, left, right, value);
        return PairResult::DAMAGED;
    }
}
