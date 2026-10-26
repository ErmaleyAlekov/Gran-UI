#include "gran/image/internal/inflate_internal.hpp"

namespace gran::image::internal
{
    // Байты подкачиваются в 32-битный буфер; за концом данных — failed, а не чтение чужой памяти
    // Bytes are pulled into a 32-bit buffer; beyond the data end — failed, not a read of foreign memory
    std::uint32_t readBits(BitReader& reader, int need) noexcept
    {
        while (reader.count < need)
        {
            if (reader.position >= reader.size)
            {
                reader.failed = true;
                return 0;
            }
            reader.buffer |= static_cast<std::uint32_t>(reader.data[reader.position++]) << reader.count;
            reader.count += 8;
        }
        std::uint32_t value = reader.buffer & ((1u << need) - 1u);
        reader.buffer >>= need;
        reader.count -= need;
        return value;
    }

    /*
    ** Канонический код: число кодов каждой длины и символы, упорядоченные по (длина, символ).
    ** Переподписка (кодов больше, чем помещается) — ошибка; неполный код допустим (как в zlib).
    **
    ** A canonical code: the code count per length and the symbols ordered by (length, symbol).
    ** Oversubscription (more codes than fit) is an error; an incomplete code is allowed (as in zlib).
    */
    bool buildHuffman(Huffman& code, const std::uint16_t* lengths, std::size_t count)
    {
        std::uint16_t offsets[16] = {};
        int left = 1;

        for (std::uint16_t& value : code.counts)
            value = 0;
        for (std::size_t symbol = 0; symbol < count; ++symbol)
            ++code.counts[lengths[symbol]];
        for (int length = 1; length < 16; ++length)
        {
            left = left * 2 - code.counts[length];
            if (left < 0)
                return false;
        }
        for (int length = 1; length < 15; ++length)
            offsets[length + 1] = static_cast<std::uint16_t>(offsets[length] + code.counts[length]);
        code.symbols.assign(count, 0);
        for (std::size_t symbol = 0; symbol < count; ++symbol)
        {
            if (lengths[symbol] != 0)
                code.symbols[offsets[lengths[symbol]]++] = static_cast<std::uint16_t>(symbol);
        }
        return true;
    }

    // Код растёт по биту; на каждой длине проверяется, попал ли он в диапазон кодов этой длины
    // The code grows bit by bit; at each length it is checked whether it fell into the range of codes of that length
    int decodeSymbol(BitReader& reader, const Huffman& code) noexcept
    {
        int value = 0;
        int first = 0;
        int index = 0;

        for (int length = 1; length < 16; ++length)
        {
            value |= static_cast<int>(readBits(reader, 1));
            int count = code.counts[length];
            if (value - count < first)
                return code.symbols[static_cast<std::size_t>(index + (value - first))];
            index += count;
            first = (first + count) << 1;
            value <<= 1;
        }
        return -1;
    }
}
