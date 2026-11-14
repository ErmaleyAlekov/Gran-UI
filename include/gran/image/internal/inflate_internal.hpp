#ifndef GRAN_IMAGE_INTERNAL_INFLATE_INTERNAL_HPP
#define GRAN_IMAGE_INTERNAL_INFLATE_INTERNAL_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gran::image::internal
{
    // Чтение битов deflate (младшие биты первыми); failed — данные кончились / Reading deflate bits (low bits first); failed — the data ended
    struct BitReader
    {
        const std::uint8_t* data = nullptr;
        std::size_t size = 0;
        std::size_t position = 0;
        std::uint32_t buffer = 0;
        int count = 0;
        bool failed = false;
    };

    // Самый длинный код DEFLATE (RFC 1951 §3.2.7): длины 0..15 / The longest DEFLATE code (RFC 1951 §3.2.7): lengths 0..15
    constexpr std::uint16_t MAX_CODE_BITS = 15;

    /*
    ** Предел одного чтения битов: буфер 32-битный, и до чтения в нём может остаться до 7 бит,
    ** поэтому больше 24 бит за раз без переполнения сдвига не дочитать. DEFLATE просит не больше 13.
    **
    ** The limit of one bit read: the buffer is 32-bit and up to 7 bits may remain in it before the read,
    ** so more than 24 bits at once cannot be pulled in without a shift overflow. DEFLATE asks for at most 13.
    */
    constexpr int MAX_READ_BITS = 24;

    // Канонический код Хаффмана: число кодов каждой длины и символы по порядку / A canonical Huffman code: the code count per length and the symbols in order
    struct Huffman
    {
        std::uint16_t counts[MAX_CODE_BITS + 1] = {};
        std::vector<std::uint16_t> symbols;
    };

    // Вывод с пределом размера / The output with a size limit
    struct InflateOutput
    {
        std::vector<std::uint8_t>& bytes;
        std::size_t limit;
    };

    // ---- inflate_bits.cpp ----

    // need бит (0..MAX_READ_BITS); вне предела или при нехватке данных — failed и 0
    // need bits (0..MAX_READ_BITS); out of the limit or when the data runs out — failed and 0
    std::uint32_t readBits(BitReader& reader, int need) noexcept;

    // Код по длинам; false — длина больше MAX_CODE_BITS или длины противоречивы (переподписка)
    // A code by lengths; false — a length above MAX_CODE_BITS or the lengths are contradictory (oversubscribed)
    bool buildHuffman(Huffman& code, const std::uint16_t* lengths, std::size_t count);

    // Символ по коду (побитно, как в puff); −1 — неверный код / A symbol by the code (bit by bit, as in puff); −1 — an invalid code
    int decodeSymbol(BitReader& reader, const Huffman& code) noexcept;

    // ---- inflate_blocks.cpp ----

    // Блок без сжатия / A stored block
    bool storedBlock(BitReader& reader, InflateOutput& output);

    // Сжатый блок с готовыми кодами длин и расстояний / A compressed block with ready length and distance codes
    bool codesBlock(BitReader& reader, InflateOutput& output, const Huffman& lengths, const Huffman& distances);

    // ---- inflate_tables.cpp ----

    // Фиксированные коды (тип 1) / The fixed codes (type 1)
    bool fixedBlock(BitReader& reader, InflateOutput& output);

    // Динамические коды из заголовка блока (тип 2) / The dynamic codes from the block header (type 2)
    bool dynamicBlock(BitReader& reader, InflateOutput& output);
}

#endif
