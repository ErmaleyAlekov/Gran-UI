#include "gran/image/internal/jpeg_internal.hpp"

#include <algorithm>

namespace gran::image::internal
{
    // Зигзаг T.81 (рис. A.6) и защитный хвост / The T.81 zigzag (figure A.6) and the guard tail
    const std::uint8_t JPEG_ZIGZAG[80] = {
        0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
        12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63};

    namespace
    {
        // Все 9-битные префиксы, начинающиеся с кода code длины length, ведут к символу
        // All the 9-bit prefixes starting with the code code of the length length lead to the symbol
        void fillFast(HuffmanTable& table, std::int32_t code, int length, std::uint8_t symbol) noexcept
        {
            int shift = HUFFMAN_FAST_BITS - length;
            std::int32_t first = code << shift;

            for (std::int32_t index = 0; index < (1 << shift); ++index)
                table.fast[first + index] = static_cast<std::uint16_t>((length << 8) | symbol);
        }
    }

    /*
    ** Канонические коды (T.81, C.2): коды одной длины идут подряд, при переходе к следующей
    ** длине код удваивается. Код, не поместившийся в свою длину, — испорченная таблица.
    **
    ** The canonical codes (T.81, C.2): the codes of one length go in a row, on moving to the next
    ** length the code doubles. A code that does not fit its length is a damaged table.
    */
    bool buildHuffman(const std::uint8_t* counts, const std::uint8_t* values, HuffmanTable& table) noexcept
    {
        std::int32_t code = 0;
        int symbol = 0;

        std::fill(table.fast, table.fast + (1 << HUFFMAN_FAST_BITS), HUFFMAN_NO_ENTRY);
        for (int length = 1; length <= 16; ++length)
        {
            table.valueOffset[length] = symbol - code;
            for (int index = 0; index < counts[length - 1]; ++index)
            {
                // Проверка до записи: код вне длины вывел бы быструю таблицу за границу
                // The check before writing: a code outside the length would take the fast table beyond its bound
                if (code >= (1 << length))
                    return false;
                table.values[symbol] = values[symbol];
                if (length <= HUFFMAN_FAST_BITS)
                    fillFast(table, code, length, values[symbol]);
                code += 1;
                symbol += 1;
            }
            table.maxCode[length] = code - 1;
            if (counts[length - 1] == 0)
                table.maxCode[length] = -1;
            code <<= 1;
        }
        table.present = true;
        return true;
    }
}
