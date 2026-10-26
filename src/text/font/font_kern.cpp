#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    namespace
    {
        // Двоичный поиск пары в подтаблице формата 0 (пары по 6 байт с 14-го) / A binary pair search in a format 0 subtable (6-byte pairs from byte 14)
        float findPair(const FontReader& reader, std::size_t subtable, std::uint32_t key) noexcept
        {
            std::size_t low = 0;
            std::size_t high = reader.u16(subtable + 6);

            while (low < high)
            {
                std::size_t middle = (low + high) / 2;
                std::uint32_t pair = reader.u32(subtable + 14 + middle * 6);
                if (pair == key)
                    return reader.i16(subtable + 14 + middle * 6 + 4);
                if (pair < key)
                    low = middle + 1;
                else
                    high = middle;
            }
            return 0.0f;
        }
    }

    /*
    ** Таблица kern Microsoft (версия 0): первая горизонтальная подтаблица формата 0 без
    ** поперечного сдвига; пары упорядочены по (левый << 16 | правый).
    **
    ** The Microsoft kern table (version 0): the first horizontal format 0 subtable without
    ** a cross-stream shift; the pairs are ordered by (left << 16 | right).
    */
    float kernTableValue(const FontReader& reader, const TableSpan& kern, std::uint16_t left, std::uint16_t right) noexcept
    {
        std::size_t position = kern.offset + 4;
        std::uint16_t tables = reader.u16(kern.offset + 2);

        if (reader.u16(kern.offset) != 0)
            return 0.0f;
        for (std::uint16_t table = 0; table < tables; ++table)
        {
            std::uint16_t coverage = reader.u16(position + 4);
            if ((coverage >> 8) == 0 && (coverage & 0x0005) == 0x0001)
                return findPair(reader, position, (static_cast<std::uint32_t>(left) << 16) | right);
            std::uint16_t length = reader.u16(position + 2);
            if (length == 0)
                return 0.0f;
            position += length;
        }
        return 0.0f;
    }
}
