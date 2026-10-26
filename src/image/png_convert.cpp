#include "gran/image/internal/png_internal.hpp"

namespace gran::image::internal
{
    namespace
    {
        // Отсчёт → 0..255 (16 бит — старший байт, малые глубины растягиваются) / A sample → 0..255 (16 bits — the high byte, small depths are stretched)
        std::uint8_t to8(std::uint32_t sample, std::uint8_t depth) noexcept
        {
            if (depth == 16)
                return static_cast<std::uint8_t>(sample >> 8);
            return static_cast<std::uint8_t>(sample * 255u / ((1u << depth) - 1u));
        }

        /*
        ** Пиксель x в RGBA8 (без предумножения): серый, RGB, палитра, серый с альфой, RGBA.
        **
        ** The pixel x in RGBA8 (without premultiplication): gray, RGB, a palette, gray with alpha, RGBA.
        */
        void pixelAt(const PngChunks& chunks, const std::uint8_t* row, std::size_t x, std::uint8_t* rgba)
        {
            const PngHeader& header = chunks.header;
            std::size_t channels = channelsOf(header.colorType);
            std::uint8_t samples[4] = {0, 0, 0, 255};

            // Предел 4 — защита массива samples от неизвестного типа цвета (данные файла недоверенные)
            // The limit 4 guards the samples array against an unknown color type (the file data is untrusted)
            for (std::size_t channel = 0; channel < channels && channel < 4; ++channel)
                samples[channel] = to8(rawSample(row, x * channels + channel, header.depth), header.depth);
            rgba[0] = samples[0];
            rgba[1] = samples[1];
            rgba[2] = samples[2];
            rgba[3] = 255;
            if (header.colorType == 0 || header.colorType == 4)
                rgba[1] = rgba[2] = samples[0];
            if (header.colorType == 4)
                rgba[3] = samples[1];
            if (header.colorType == 6)
                rgba[3] = samples[3];
            if ((header.colorType == 0 || header.colorType == 2) && keyedOut(chunks, row, x, channels))
                rgba[3] = 0;
            if (header.colorType == 3)
                palettePixel(chunks, row, x, rgba);
        }
    }

    // Пиксели построчно с предумножением альфы (так их смешивает шейдер) / The pixels row by row with alpha premultiplication (so the shader blends them)
    void convertRows(const PngChunks& chunks, const std::vector<std::uint8_t>& raw, Image& image)
    {
        std::size_t stride = rowBytes(chunks.header) + 1;

        image.width = chunks.header.width;
        image.height = chunks.header.height;
        image.pixels.resize(static_cast<std::size_t>(image.width) * image.height * 4);
        for (std::size_t y = 0; y < image.height; ++y)
        {
            const std::uint8_t* row = raw.data() + y * stride + 1;
            for (std::size_t x = 0; x < image.width; ++x)
            {
                std::uint8_t* pixel = image.pixels.data() + (y * image.width + x) * 4;
                pixelAt(chunks, row, x, pixel);
                for (int channel = 0; channel < 3; ++channel)
                    pixel[channel] = static_cast<std::uint8_t>((pixel[channel] * pixel[3] + 127) / 255);
            }
        }
    }
}
