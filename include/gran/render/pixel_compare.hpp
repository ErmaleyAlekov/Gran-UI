#ifndef GRAN_RENDER_PIXEL_COMPARE_HPP
#define GRAN_RENDER_PIXEL_COMPARE_HPP

#include "gran/image/image.hpp"

namespace gran::render
{
    // Число пикселей за допуском и наибольшая разница канала; размеры проверяются отдельно.
    // Pixel count outside tolerance and the largest channel delta; dimensions are checked separately.
    struct PixelComparison
    {
        bool sameSize = false;
        std::size_t differentPixels = 0;
        unsigned maxDelta = 0;
    };

    // RGBA8, абсолютный допуск на канал (0..255); неверная длина массива считается несовпадением.
    // RGBA8, absolute per-channel tolerance (0..255); invalid array length counts as a mismatch.
    PixelComparison comparePixels(const image::Image& actual, const image::Image& expected, unsigned tolerance);
}

#endif
