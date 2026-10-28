#include "gran/image/internal/jpeg_internal.hpp"

namespace gran::image::internal
{
    /*
    ** Буфер держит биты в старших разрядах. Байт 0xFF 0x00 — это 0xFF; 0xFF с другим байтом — маркер:
    ** чтение встаёт перед ним (позиция на 0xFF) и дальше добавляет нули, как и конец данных.
    **
    ** The buffer keeps the bits in the high-order positions. The bytes 0xFF 0x00 mean 0xFF; 0xFF with another byte is a marker:
    ** reading stops before it (the position is at 0xFF) and then adds zeros, as does the end of the data.
    */
    void fillBits(JpegReader& reader) noexcept
    {
        while (reader.count <= 24)
        {
            std::uint32_t byte = 0;

            if (reader.marker || reader.pos >= reader.size)
                reader.padding += 8;
            if (!reader.marker && reader.pos < reader.size)
            {
                byte = reader.data[reader.pos];
                if (byte != 0xFF)
                    reader.pos += 1;
                else if (reader.pos + 1 < reader.size && reader.data[reader.pos + 1] == 0x00)
                    reader.pos += 2;
                else
                {
                    reader.marker = true;
                    reader.padding += 8;
                    byte = 0;
                }
            }
            reader.buffer |= byte << (24 - reader.count);
            reader.count += 8;
        }
    }

    // Старшие count бит буфера / The count high-order bits of the buffer
    std::uint32_t takeBits(JpegReader& reader, int count) noexcept
    {
        std::uint32_t value = 0;

        if (count <= 0)
            return 0;
        fillBits(reader);
        value = reader.buffer >> (32 - count);
        reader.buffer <<= count;
        reader.count -= count;
        if (reader.padding > reader.count)
            reader.padding = reader.count;
        return value;
    }

    /*
    ** EXTEND: значение из size бит со старшим нулём — отрицательное (v − 2^size + 1).
    ** Например, size = 2: 00 → −3, 01 → −2, 10 → 2, 11 → 3.
    **
    ** EXTEND: a value of size bits with a zero high bit is negative (v − 2^size + 1).
    ** For example, size = 2: 00 → −3, 01 → −2, 10 → 2, 11 → 3.
    */
    int receiveExtend(JpegReader& reader, int size) noexcept
    {
        int value = 0;

        if (size <= 0)
            return 0;
        value = static_cast<int>(takeBits(reader, size));
        if (value < (1 << (size - 1)))
            value += 1 - (1 << size);
        return value;
    }

    /*
    ** Короткие коды (до 9 бит) — одним просмотром быстрой таблицы, длинные — сравнением
    ** с наибольшим кодом каждой длины (канонические коды: у более длинных кодов значения больше).
    **
    ** Short codes (up to 9 bits) — by one look-up of the fast table, long ones — by comparing
    ** with the largest code of every length (canonical codes: longer codes have larger values).
    */
    int decodeHuffman(JpegReader& reader, const HuffmanTable& table) noexcept
    {
        std::uint16_t entry = 0;

        fillBits(reader);
        entry = table.fast[reader.buffer >> (32 - HUFFMAN_FAST_BITS)];
        if (entry != HUFFMAN_NO_ENTRY)
        {
            takeBits(reader, entry >> 8);
            return entry & 0xFF;
        }
        for (int length = HUFFMAN_FAST_BITS + 1; length <= 16; ++length)
        {
            std::int32_t code = static_cast<std::int32_t>(reader.buffer >> (32 - length));

            if (code <= table.maxCode[length])
            {
                takeBits(reader, length);
                return table.values[(code + table.valueOffset[length]) & 0xFF];
            }
        }
        return -1;
    }

    /*
    ** Перезапуск: недочитанные биты байта отбрасываются, маркер RSTn (0xFF 0xD0–0xD7)
    ** пропускается. Нет маркера — данные читаются дальше как есть (терпимость к испорченным файлам).
    **
    ** A restart: the unread bits of the byte are dropped, the RSTn marker (0xFF 0xD0–0xD7)
    ** is skipped. No marker — the data is read further as it is (tolerance to damaged files).
    */
    void restartBits(JpegReader& reader) noexcept
    {
        reader.buffer = 0;
        reader.count = 0;
        reader.padding = 0;
        reader.marker = false;
        // Байты-заполнители 0xFF перед маркером / The 0xFF fill bytes before the marker
        while (reader.pos + 1 < reader.size && reader.data[reader.pos] == 0xFF && reader.data[reader.pos + 1] == 0xFF)
            reader.pos += 1;
        if (reader.pos + 1 < reader.size && reader.data[reader.pos] == 0xFF
            && reader.data[reader.pos + 1] >= 0xD0 && reader.data[reader.pos + 1] <= 0xD7)
            reader.pos += 2;
    }
}
