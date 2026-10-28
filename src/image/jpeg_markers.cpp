#include "gran/image/internal/jpeg_internal.hpp"

#include <cstring>

namespace gran::image::internal
{
    // Два байта big-endian / Two big-endian bytes
    std::uint32_t readWord(const std::uint8_t* data) noexcept
    {
        return (static_cast<std::uint32_t>(data[0]) << 8) | data[1];
    }

    /*
    ** DQT: несколько таблиц подряд, у каждой байт «точность | номер», затем 64 значения
    ** в порядке зигзага (по 1 или 2 байта). Храним в естественном порядке — как коэффициенты.
    **
    ** DQT: several tables in a row, each with the byte "precision | number", then 64 values
    ** in the zigzag order (1 or 2 bytes each). Stored in the natural order — as the coefficients.
    */
    void readQuant(JpegFrame& frame, const std::uint8_t* segment, std::size_t length)
    {
        std::size_t offset = 0;

        while (offset < length)
        {
            std::size_t precision = segment[offset] >> 4;
            std::size_t number = segment[offset] & 15;
            std::size_t bytes = 64 * (precision + 1);

            if (precision > 1 || number > 3 || offset + 1 + bytes > length)
                throwBadImage(*frame.source, "DQT");
            for (std::size_t k = 0; k < 64; ++k)
            {
                std::uint32_t value = segment[offset + 1 + k];

                if (precision == 1)
                    value = readWord(segment + offset + 1 + 2 * k);
                frame.quant[number][JPEG_ZIGZAG[k]] = static_cast<std::uint16_t>(value);
            }
            frame.quantPresent[number] = true;
            offset += 1 + bytes;
        }
    }

    /*
    ** DHT: несколько таблиц подряд: байт «класс | номер» (класс 0 — DC, 1 — AC), 16 чисел кодов
    ** каждой длины, затем символы (всего не больше 256).
    **
    ** DHT: several tables in a row: the byte "class | number" (class 0 — DC, 1 — AC), 16 code counts
    ** of every length, then the symbols (no more than 256 in all).
    */
    void readHuffmanTables(JpegFrame& frame, const std::uint8_t* segment, std::size_t length)
    {
        std::size_t offset = 0;

        while (offset < length)
        {
            std::size_t tableClass = segment[offset] >> 4;
            std::size_t number = segment[offset] & 15;
            std::size_t total = 0;
            HuffmanTable* table = &frame.dc[number & 3];

            if (tableClass > 1 || number > 3 || offset + 17 > length)
                throwBadImage(*frame.source, "DHT");
            for (std::size_t index = 0; index < 16; ++index)
                total += segment[offset + 1 + index];
            if (tableClass == 1)
                table = &frame.ac[number];
            if (total > 256 || offset + 17 + total > length
                || !buildHuffman(segment + offset + 1, segment + offset + 17, *table))
                throwBadImage(*frame.source, "DHT");
            offset += 17 + total;
        }
    }

    // DRI: число MCU между маркерами RSTn (0 — без перезапусков) / DRI: the MCU count between the RSTn markers (0 — no restarts)
    void readRestartInterval(JpegFrame& frame, const std::uint8_t* segment, std::size_t length)
    {
        if (length < 2)
            throwBadImage(*frame.source, "DRI");
        frame.restartInterval = readWord(segment);
    }

    /*
    ** APP14 «Adobe»: 5 байт подписи, версия (2), флаги (4), байт преобразования:
    ** 0 — компоненты без преобразования (RGB), 1 — YCbCr. Не Adobe — сегмент пропускается.
    **
    ** APP14 "Adobe": 5 signature bytes, the version (2), the flags (4), the transform byte:
    ** 0 — the components are not transformed (RGB), 1 — YCbCr. Not Adobe — the segment is skipped.
    */
    void readAdobe(JpegFrame& frame, const std::uint8_t* segment, std::size_t length) noexcept
    {
        if (length >= 12 && std::memcmp(segment, "Adobe", 5) == 0)
            frame.adobeTransform = segment[11];
    }
}
