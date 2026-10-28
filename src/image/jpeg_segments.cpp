#include "gran/image/internal/jpeg_internal.hpp"

namespace gran::image::internal
{
    namespace
    {
        // Маркеры без длины: RSTn, TEM и байт-заполнитель / The markers without a length: RSTn, TEM and a fill byte
        bool standalone(std::uint8_t marker) noexcept
        {
            return (marker >= 0xD0 && marker <= 0xD7) || marker == 0x01 || marker == 0xFF || marker == 0x00;
        }

        // Позиция байта 0xFF следующего маркера с длиной или EOI; нет — size / The position of the 0xFF byte of the next marker with a length or EOI; none — size
        std::size_t nextMarker(const std::uint8_t* data, std::size_t size, std::size_t pos) noexcept
        {
            while (pos + 1 < size)
            {
                if (data[pos] == 0xFF && !standalone(data[pos + 1]))
                    return pos;
                pos += 1;
            }
            return size;
        }

        /*
        ** Сегмент по маркеру. SOF арифметического кодирования, иерархические и без потерь
        ** не поддерживаются; APPn (кроме Adobe), COM и прочие — пропускаются.
        **
        ** A segment by its marker. The arithmetic, hierarchical and lossless SOFs
        ** are not supported; APPn (except Adobe), COM and the others are skipped.
        */
        void readSegment(JpegFrame& frame, std::uint8_t marker, const std::uint8_t* segment, std::size_t length)
        {
            if (marker == 0xDB)
                readQuant(frame, segment, length);
            else if (marker == 0xC4)
                readHuffmanTables(frame, segment, length);
            else if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2)
                readFrameHeader(frame, marker, segment, length);
            else if (marker >= 0xC3 && marker <= 0xCF && marker != 0xC8 && marker != 0xCC)
                throwBadImage(*frame.source, "unsupported coding");
            else if (marker == 0xDD)
                readRestartInterval(frame, segment, length);
            else if (marker == 0xEE)
                readAdobe(frame, segment, length);
        }
    }

    /*
    ** Проход по сегментам: маркер, длина (включает себя), данные. Скан SOS сам читает свои
    ** энтропийные данные и говорит, где они кончились. Обрезанный после скана файл (нет EOI)
    ** допустим — недостающие блоки останутся серыми, как в браузерах.
    **
    ** A pass over the segments: the marker, the length (including itself), the data. An SOS scan reads its own
    ** entropy data and tells where it ended. A file truncated after a scan (no EOI)
    ** is allowed — the missing blocks stay gray, as in browsers.
    */
    void readSegments(JpegFrame& frame, const std::uint8_t* data, std::size_t size)
    {
        std::size_t pos = nextMarker(data, size, 2);

        while (pos + 1 < size && data[pos + 1] != 0xD9)
        {
            std::uint8_t marker = data[pos + 1];
            std::size_t length = 0;

            if (pos + 4 > size || readWord(data + pos + 2) < 2 || pos + 2 + readWord(data + pos + 2) > size)
                throwBadImage(*frame.source, "segment length");
            length = readWord(data + pos + 2) - 2;
            if (marker == 0xDA)
                pos = readScan(frame, data, size, pos + 4);
            else
            {
                readSegment(frame, marker, data + pos + 4, length);
                pos += 4 + length;
            }
            pos = nextMarker(data, size, pos);
        }
    }
}
