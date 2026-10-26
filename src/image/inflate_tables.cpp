#include "gran/image/internal/inflate_internal.hpp"

namespace gran::image::internal
{
    namespace
    {
        // Порядок длин кодов длин в заголовке динамического блока / The order of the code length code lengths in the dynamic block header
        const std::uint8_t CODE_ORDER[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

        // Серия длин: значение и число повторов / A run of lengths: the value and the repeat count
        struct Run
        {
            std::uint16_t value;
            std::size_t repeat;
        };

        /*
        ** Символ кода длин: 0–15 — длина, 16 — повтор предыдущей 3–6 раз, 17 — 3–10 нулей,
        ** 18 — 11–138 нулей. Повтор без предыдущей длины — ошибка.
        **
        ** A code length symbol: 0–15 — a length, 16 — repeat the previous 3–6 times, 17 — 3–10 zeros,
        ** 18 — 11–138 zeros. A repeat without a previous length is an error.
        */
        bool lengthRun(BitReader& reader, int symbol, std::uint16_t previous, bool hasPrevious, Run& run)
        {
            run = Run{0, 1};
            if (symbol >= 0 && symbol < 16)
                run.value = static_cast<std::uint16_t>(symbol);
            else if (symbol == 16 && hasPrevious)
                run = Run{previous, 3 + readBits(reader, 2)};
            else if (symbol == 17)
                run.repeat = 3 + readBits(reader, 3);
            else if (symbol == 18)
                run.repeat = 11 + readBits(reader, 7);
            else
                return false;
            return !reader.failed;
        }

        // Длины кодов литералов и расстояний; выход за total — ошибка / The literal and distance code lengths; going beyond total is an error
        bool readCodeLengths(BitReader& reader, const Huffman& code, std::uint16_t* lengths, std::size_t total)
        {
            std::size_t index = 0;
            Run run{0, 0};

            while (index < total)
            {
                std::uint16_t previous = 0;
                if (index > 0)
                    previous = lengths[index - 1];
                if (!lengthRun(reader, decodeSymbol(reader, code), previous, index > 0, run) || run.repeat > total - index)
                    return false;
                for (; run.repeat > 0; --run.repeat)
                    lengths[index++] = run.value;
            }
            return true;
        }
    }

    // Фиксированные коды RFC 1951 §3.2.6 / The fixed codes of RFC 1951 §3.2.6
    bool fixedBlock(BitReader& reader, InflateOutput& output)
    {
        std::uint16_t lengths[288];
        std::uint16_t distances[30];
        Huffman lengthCode;
        Huffman distanceCode;

        for (std::size_t symbol = 0; symbol < 288; ++symbol)
            lengths[symbol] = 8;
        for (std::size_t symbol = 144; symbol < 256; ++symbol)
            lengths[symbol] = 9;
        for (std::size_t symbol = 256; symbol < 280; ++symbol)
            lengths[symbol] = 7;
        for (std::uint16_t& length : distances)
            length = 5;
        buildHuffman(lengthCode, lengths, 288);
        buildHuffman(distanceCode, distances, 30);
        return codesBlock(reader, output, lengthCode, distanceCode);
    }

    // Заголовок динамического блока, коды длин, затем сами коды / The dynamic block header, the length codes, then the codes themselves
    bool dynamicBlock(BitReader& reader, InflateOutput& output)
    {
        std::uint16_t lengths[320] = {};
        Huffman lengthCode;
        Huffman distanceCode;
        std::size_t literals = readBits(reader, 5) + 257;
        std::size_t distances = readBits(reader, 5) + 1;
        std::size_t codes = readBits(reader, 4) + 4;

        if (reader.failed || literals > 286 || distances > 30)
            return false;
        for (std::size_t index = 0; index < codes; ++index)
            lengths[CODE_ORDER[index]] = static_cast<std::uint16_t>(readBits(reader, 3));
        Huffman codeLengths;
        if (!buildHuffman(codeLengths, lengths, 19) || !readCodeLengths(reader, codeLengths, lengths, literals + distances))
            return false;
        if (lengths[256] == 0 || !buildHuffman(lengthCode, lengths, literals)
            || !buildHuffman(distanceCode, lengths + literals, distances))
            return false;
        return codesBlock(reader, output, lengthCode, distanceCode);
    }
}
