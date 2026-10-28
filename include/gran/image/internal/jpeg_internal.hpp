#ifndef GRAN_IMAGE_INTERNAL_JPEG_INTERNAL_HPP
#define GRAN_IMAGE_INTERNAL_JPEG_INTERNAL_HPP

#include "gran/image/image.hpp"
#include "gran/image/internal/image_error.hpp"

namespace gran::image::internal
{
    // Наибольшее число компонент кадра (серый — 1, YCbCr/RGB — 3) / The largest frame component count (gray — 1, YCbCr/RGB — 3)
    constexpr std::size_t JPEG_MAX_COMPONENTS = 3;

    // Признак «нет записи» в быстрой таблице Хаффмана / The "no entry" mark in the fast Huffman table
    constexpr std::uint16_t HUFFMAN_NO_ENTRY = 0xFFFF;

    // Бит в быстрой таблице Хаффмана (коды до 9 бит — за один просмотр) / Bits of the fast Huffman table (codes up to 9 bits — in one look-up)
    constexpr int HUFFMAN_FAST_BITS = 9;

    // Порядок зигзага → естественный номер коэффициента; хвост из 63 — защита от выхода k за 63
    // The zigzag order → the natural coefficient number; the tail of 63 guards against k going past 63
    extern const std::uint8_t JPEG_ZIGZAG[80];

    /*
    ** Таблица Хаффмана (ITU T.81, F.2.2.3): быстрая таблица на 9 бит — (длина << 8) | символ,
    ** для длинных кодов — наибольший код каждой длины и сдвиг до символа (символ = values[code + offset]).
    **
    ** A Huffman table (ITU T.81, F.2.2.3): the fast 9-bit table — (length << 8) | symbol,
    ** for long codes — the largest code of every length and the shift to the symbol (symbol = values[code + offset]).
    */
    struct HuffmanTable
    {
        std::uint16_t fast[1 << HUFFMAN_FAST_BITS] = {};
        std::int32_t maxCode[18] = {};
        std::int32_t valueOffset[18] = {};
        std::uint8_t values[256] = {};
        bool present = false;
    };

    /*
    ** Компонента кадра: множители выборки h/v, номера таблиц, блоки коэффициентов (по 64, в
    ** естественном порядке, ещё не умноженные на квантование) и после IDCT — плоскость байтов.
    ** Блоков — до границы MCU, поэтому плоскость шире видимой части (width × height).
    **
    ** A frame component: the sampling factors h/v, the table numbers, the coefficient blocks (64 each, in
    ** the natural order, not multiplied by the quantization yet) and after IDCT — a byte plane.
    ** The blocks go up to the MCU border, so the plane is wider than the visible part (width × height).
    */
    struct JpegComponent
    {
        std::uint8_t id = 0;
        std::uint8_t h = 1;
        std::uint8_t v = 1;
        std::uint8_t quant = 0;
        std::uint8_t dcTable = 0;
        std::uint8_t acTable = 0;
        std::uint32_t blocksWide = 0;
        std::uint32_t blocksHigh = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        int dcPredictor = 0;
        std::vector<std::int16_t> coefficients;
        std::vector<std::uint8_t> plane;
    };

    /*
    ** Чтение битов энтропийных данных: байт 0xFF 0x00 — это 0xFF, иначе 0xFF начинает маркер —
    ** чтение останавливается на нём и дальше отдаёт нули (обрезанный файл не читается за границей).
    **
    ** Reading the entropy-coded bits: the bytes 0xFF 0x00 mean 0xFF, otherwise 0xFF starts a marker —
    ** reading stops at it and then yields zeros (a truncated file is not read beyond its end).
    */
    struct JpegReader
    {
        const std::uint8_t* data = nullptr;
        std::size_t size = 0;
        std::size_t pos = 0;
        std::uint32_t buffer = 0;
        int count = 0;
        // Сколько из count бит буфера — добавленные нули (маркер или конец данных) / How many of the count buffer bits are added zeros (a marker or the data end)
        int padding = 0;
        bool marker = false;
    };

    // Заголовок скана (SOS): компоненты скана и параметры прогрессивного режима / The scan header (SOS): the scan components and the progressive parameters
    struct JpegScan
    {
        std::uint8_t components[JPEG_MAX_COMPONENTS] = {};
        std::size_t count = 0;
        int spectralStart = 0;
        int spectralEnd = 63;
        int approxHigh = 0;
        int approxLow = 0;
    };

    // Состояние декодера: кадр, таблицы, компоненты / The decoder state: the frame, the tables, the components
    struct JpegFrame
    {
        const std::string* source = nullptr;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        bool progressive = false;
        bool frameRead = false;
        bool scanRead = false;
        std::uint8_t hMax = 1;
        std::uint8_t vMax = 1;
        std::uint32_t mcusWide = 0;
        std::uint32_t mcusHigh = 0;
        std::uint32_t restartInterval = 0;
        // Преобразование цвета из APP14 Adobe (-1 — маркера нет) / The color transform from APP14 Adobe (-1 — no marker)
        int adobeTransform = -1;
        // Оставшиеся блоки серии конца полосы (EOBRUN, прогрессивный AC) / The remaining end-of-band run blocks (EOBRUN, progressive AC)
        std::uint32_t eobRun = 0;
        JpegComponent components[JPEG_MAX_COMPONENTS];
        std::size_t componentCount = 0;
        std::uint16_t quant[4][64] = {};
        bool quantPresent[4] = {};
        HuffmanTable dc[4];
        HuffmanTable ac[4];
    };

    // ---- jpeg_bits.cpp ----

    // Дочитать буфер до 25+ бит / Top up the buffer to 25+ bits
    void fillBits(JpegReader& reader) noexcept;

    // Следующие count бит (0–16) старшими вперёд / The next count bits (0–16), the most significant first
    std::uint32_t takeBits(JpegReader& reader, int count) noexcept;

    // Знаковое значение из size бит (T.81, F.2.2.1 EXTEND) / A signed value of size bits (T.81, F.2.2.1 EXTEND)
    int receiveExtend(JpegReader& reader, int size) noexcept;

    // Символ по таблице Хаффмана; -1 — такого кода нет (данные испорчены) / A symbol by the Huffman table; -1 — no such code (the data is damaged)
    int decodeHuffman(JpegReader& reader, const HuffmanTable& table) noexcept;

    // Перезапуск (RSTn): отбросить биты и пропустить маркер / A restart (RSTn): drop the bits and skip the marker
    void restartBits(JpegReader& reader) noexcept;

    // ---- jpeg_huffman.cpp ----

    // Таблица из числа кодов каждой длины (16) и символов; false — коды не помещаются в длины
    // A table from the code count of every length (16) and the symbols; false — the codes do not fit the lengths
    bool buildHuffman(const std::uint8_t* counts, const std::uint8_t* values, HuffmanTable& table) noexcept;

    // ---- jpeg_markers.cpp: сегменты таблиц / the table segments ----

    // Слово big-endian / A big-endian word
    std::uint32_t readWord(const std::uint8_t* data) noexcept;

    // DQT: таблицы квантования (8 и 16 бит) / DQT: the quantization tables (8 and 16 bits)
    void readQuant(JpegFrame& frame, const std::uint8_t* segment, std::size_t length);

    // DHT: таблицы Хаффмана DC и AC / DHT: the DC and AC Huffman tables
    void readHuffmanTables(JpegFrame& frame, const std::uint8_t* segment, std::size_t length);

    // DRI: интервал перезапуска / DRI: the restart interval
    void readRestartInterval(JpegFrame& frame, const std::uint8_t* segment, std::size_t length);

    // APP14 «Adobe»: преобразование цвета / APP14 "Adobe": the color transform
    void readAdobe(JpegFrame& frame, const std::uint8_t* segment, std::size_t length) noexcept;

    // ---- jpeg_frame.cpp ----

    // SOF0/1/2: размеры, компоненты, режим / SOF0/1/2: the sizes, the components, the mode
    void readFrameHeader(JpegFrame& frame, std::uint8_t marker, const std::uint8_t* segment, std::size_t length);

    // Пределы размеров, число MCU, блоки компонент / The size limits, the MCU count, the component blocks
    void layoutComponents(JpegFrame& frame);

    // ---- jpeg_segments.cpp ----

    // Сегменты от SOI до EOI (или конца данных) / The segments from SOI to EOI (or the end of the data)
    void readSegments(JpegFrame& frame, const std::uint8_t* data, std::size_t size);

    // ---- jpeg_scan.cpp ----

    // SOS: заголовок и энтропийные данные; возвращает позицию после данных / SOS: the header and the entropy data; returns the position after the data
    std::size_t readScan(JpegFrame& frame, const std::uint8_t* data, std::size_t size, std::size_t pos);

    // ---- jpeg_scan_header.cpp ----

    // Заголовок скана с проверкой параметров и наличия таблиц / The scan header with checking the parameters and the tables
    void readScanHeader(JpegFrame& frame, const std::uint8_t* segment, std::size_t length, JpegScan& scan);

    // ---- jpeg_blocks.cpp ----

    // Один блок скана в нужном режиме (последовательный, DC/AC первый проход или уточнение)
    // One scan block in the right mode (sequential, the DC/AC first pass or a refinement)
    void decodeBlock(JpegFrame& frame, const JpegScan& scan, JpegComponent& component, JpegReader& reader,
        std::int16_t* block);

    // ---- jpeg_ac.cpp: прогрессивные AC-сканы / the progressive AC scans ----

    // Первый проход AC (Ah = 0): значения со сдвигом Al и серии конца полосы / The AC first pass (Ah = 0): the values shifted by Al and the end-of-band runs
    void decodeAcFirst(JpegFrame& frame, const JpegScan& scan, const HuffmanTable& table, JpegReader& reader,
        std::int16_t* block);

    // Уточнение AC (Ah > 0): новые ±1 << Al и биты уточнения прежних / The AC refinement (Ah > 0): new ±1 << Al and the refinement bits of the former ones
    void decodeAcRefine(JpegFrame& frame, const JpegScan& scan, const HuffmanTable& table, JpegReader& reader,
        std::int16_t* block);

    // ---- jpeg_idct.cpp ----

    // Коэффициенты компоненты × квантование → обратное ДКП → плоскость байтов / The component coefficients × quantization → inverse DCT → a byte plane
    void idctComponent(JpegComponent& component, const std::uint16_t* quant);

    // ---- jpeg_color.cpp ----

    // Плоскости → RGBA8 (серый, YCbCr с билинейной досэмплировкой цветности, RGB) / The planes → RGBA8 (gray, YCbCr with bilinear chroma upsampling, RGB)
    void convertJpeg(const JpegFrame& frame, Image& image);
}

#endif
