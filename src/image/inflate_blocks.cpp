#include "gran/image/internal/inflate_internal.hpp"

namespace gran::image::internal
{
    namespace
    {
        // Базы и дополнительные биты длин (символы 257–285) и расстояний (0–29) по RFC 1951
        // The bases and extra bits of lengths (symbols 257–285) and distances (0–29) per RFC 1951
        const std::uint16_t LENGTH_BASE[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
            67, 83, 99, 115, 131, 163, 195, 227, 258};
        const std::uint8_t LENGTH_EXTRA[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4,
            5, 5, 5, 5, 0};
        const std::uint16_t DISTANCE_BASE[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385,
            513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
        const std::uint8_t DISTANCE_EXTRA[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
            10, 11, 11, 12, 12, 13, 13};

        /*
        ** Повтор: длина по символу, расстояние по второму коду; копия байт за байтом
        ** (источник и приёмник могут перекрываться — так кодируются повторы одного байта).
        **
        ** A repeat: the length by the symbol, the distance by the second code; a byte by byte copy
        ** (the source and the destination may overlap — that is how repeats of one byte are encoded).
        */
        bool copyMatch(BitReader& reader, InflateOutput& output, int symbol, const Huffman& distances)
        {
            std::size_t index = static_cast<std::size_t>(symbol - 257);

            if (index >= 29)
                return false;
            std::size_t length = LENGTH_BASE[index] + readBits(reader, LENGTH_EXTRA[index]);
            int code = decodeSymbol(reader, distances);
            if (code < 0 || code >= 30 || reader.failed)
                return false;
            std::size_t distance = DISTANCE_BASE[code] + readBits(reader, DISTANCE_EXTRA[code]);
            std::vector<std::uint8_t>& bytes = output.bytes;
            if (reader.failed || distance > bytes.size() || length > output.limit - bytes.size())
                return false;
            std::size_t from = bytes.size() - distance;
            for (std::size_t offset = 0; offset < length; ++offset)
                bytes.push_back(bytes[from + offset]);
            return true;
        }
    }

    // Блок без сжатия: выравнивание на байт, LEN и его дополнение NLEN, затем байты как есть
    // A stored block: alignment to a byte, LEN and its complement NLEN, then the bytes as they are
    bool storedBlock(BitReader& reader, InflateOutput& output)
    {
        reader.buffer = 0;
        reader.count = 0;
        if (reader.size - reader.position < 4)
            return false;
        const std::uint8_t* header = reader.data + reader.position;
        std::size_t length = header[0] | (header[1] << 8);
        std::size_t complement = header[2] | (header[3] << 8);
        reader.position += 4;
        if (length != (~complement & 0xFFFFu) || length > reader.size - reader.position
            || length > output.limit - output.bytes.size())
            return false;
        output.bytes.insert(output.bytes.end(), reader.data + reader.position, reader.data + reader.position + length);
        reader.position += length;
        return true;
    }

    // Символы до 256 — байты, 256 — конец блока, больше — повтор / Symbols below 256 are bytes, 256 is the block end, above — a repeat
    bool codesBlock(BitReader& reader, InflateOutput& output, const Huffman& lengths, const Huffman& distances)
    {
        while (true)
        {
            int symbol = decodeSymbol(reader, lengths);
            if (symbol < 0 || reader.failed)
                return false;
            if (symbol == 256)
                return true;
            if (symbol < 256)
            {
                if (output.bytes.size() >= output.limit)
                    return false;
                output.bytes.push_back(static_cast<std::uint8_t>(symbol));
            }
            else if (!copyMatch(reader, output, symbol, distances))
                return false;
        }
    }
}
