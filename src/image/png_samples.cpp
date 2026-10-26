#include "gran/image/internal/png_internal.hpp"

namespace gran::image::internal
{
    // 16 бит — два байта big-endian, 8 — байт, 1/2/4 — биты от старшего / 16 bits — two big-endian bytes, 8 — a byte, 1/2/4 — bits from the high one
    std::uint32_t rawSample(const std::uint8_t* row, std::size_t index, std::uint8_t depth) noexcept
    {
        if (depth == 16)
            return (static_cast<std::uint32_t>(row[index * 2]) << 8) | row[index * 2 + 1];
        if (depth == 8)
            return row[index];
        std::size_t bit = index * depth;
        unsigned shift = 8u - depth - static_cast<unsigned>(bit % 8);
        return (row[bit / 8] >> shift) & ((1u << depth) - 1u);
    }

    // Ключ tRNS — по 2 байта на канал / The tRNS key is 2 bytes per channel
    bool keyedOut(const PngChunks& chunks, const std::uint8_t* row, std::size_t x, std::size_t channels)
    {
        const std::vector<std::uint8_t>& key = chunks.transparency;

        if (key.size() < channels * 2)
            return false;
        for (std::size_t channel = 0; channel < channels; ++channel)
        {
            std::uint32_t expected = (static_cast<std::uint32_t>(key[channel * 2]) << 8) | key[channel * 2 + 1];
            if (rawSample(row, x * channels + channel, chunks.header.depth) != expected)
                return false;
        }
        return true;
    }

    // Цвет из PLTE по индексу, альфа — из tRNS (если записана) / The color from PLTE by index, the alpha — from tRNS (if written)
    void palettePixel(const PngChunks& chunks, const std::uint8_t* row, std::size_t x, std::uint8_t* rgba)
    {
        std::size_t index = rawSample(row, x, chunks.header.depth);

        rgba[0] = 0;
        rgba[1] = 0;
        rgba[2] = 0;
        rgba[3] = 0;
        if (index * 3 + 2 >= chunks.palette.size())
            return;
        rgba[0] = chunks.palette[index * 3];
        rgba[1] = chunks.palette[index * 3 + 1];
        rgba[2] = chunks.palette[index * 3 + 2];
        rgba[3] = 255;
        if (index < chunks.transparency.size())
            rgba[3] = chunks.transparency[index];
    }
}
