#ifndef GRAN_IMAGE_INTERNAL_PNG_INTERNAL_HPP
#define GRAN_IMAGE_INTERNAL_PNG_INTERNAL_HPP

#include "gran/image/image.hpp"

namespace gran::image::internal
{
    // Заголовок IHDR / The IHDR header
    struct PngHeader
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint8_t depth = 0;
        std::uint8_t colorType = 0;
        std::uint8_t interlace = 0;
    };

    // Чанки, нужные декодеру / The chunks the decoder needs
    struct PngChunks
    {
        PngHeader header;
        std::vector<std::uint8_t> palette;
        std::vector<std::uint8_t> transparency;
        std::vector<std::uint8_t> compressed;
    };

    // ---- png_chunks.cpp ----

    // Ошибка изображения → GranException(IMAGE) / An image error → GranException(IMAGE)
    [[noreturn]] void throwBadImage(const std::string& source, const char* reason);

    // Подпись, чанки с проверкой CRC, IHDR и пределы размеров / The signature, the chunks with the CRC check, IHDR and the size limits
    void readChunks(const std::uint8_t* data, std::size_t size, const std::string& source, PngChunks& chunks);

    // Каналов в пикселе по типу цвета (0 — тип неизвестен) / Channels per pixel by the color type (0 — an unknown type)
    std::size_t channelsOf(std::uint8_t colorType) noexcept;

    // Байт в строке без байта фильтра / Bytes per row without the filter byte
    std::size_t rowBytes(const PngHeader& header) noexcept;

    // ---- png_unfilter.cpp ----

    // Снятие фильтров строк на месте; false — неизвестный фильтр / Removing the row filters in place; false — an unknown filter
    bool unfilter(std::vector<std::uint8_t>& raw, const PngHeader& header);

    // ---- png_samples.cpp ----

    // Отсчёт номер index строки с глубиной depth (1–16 бит), без масштабирования / The sample number index of the row with depth (1–16 bits), unscaled
    std::uint32_t rawSample(const std::uint8_t* row, std::size_t index, std::uint8_t depth) noexcept;

    // Прозрачен ли пиксель по ключу tRNS (серый — 1 отсчёт, RGB — 3) / Whether the pixel is transparent by the tRNS key (gray — 1 sample, RGB — 3)
    bool keyedOut(const PngChunks& chunks, const std::uint8_t* row, std::size_t x, std::size_t channels);

    // Пиксель палитры (альфа из tRNS); индекс вне палитры — прозрачный чёрный / A palette pixel (the alpha from tRNS); an index outside the palette is transparent black
    void palettePixel(const PngChunks& chunks, const std::uint8_t* row, std::size_t x, std::uint8_t* rgba);

    // ---- png_convert.cpp ----

    // Строки (с байтом фильтра в начале каждой) → RGBA8 с предумноженной альфой / The rows (with a filter byte at the start of each) → premultiplied RGBA8
    void convertRows(const PngChunks& chunks, const std::vector<std::uint8_t>& raw, Image& image);
}

#endif
