#include "gran/render/pixel_compare.hpp"

#include <algorithm>
#include <cstdlib>

namespace gran::render
{
    // Сравнение без чтения за границами, альфа проверяется наравне с RGB.
    // A bounds-safe comparison; alpha is checked along with RGB.
    PixelComparison comparePixels(const image::Image& actual, const image::Image& expected, unsigned tolerance)
    {
        PixelComparison result;
        std::uint64_t bytes = static_cast<std::uint64_t>(actual.width) * actual.height * 4;

        result.sameSize = actual.width > 0 && actual.height > 0
            && actual.width <= image::MAX_SIDE && actual.height <= image::MAX_SIDE
            && actual.width == expected.width && actual.height == expected.height
            && bytes == actual.pixels.size() && bytes == expected.pixels.size();
        if (!result.sameSize)
            return result;
        for (std::size_t offset = 0; offset < actual.pixels.size(); offset += 4)
        {
            unsigned delta = 0;
            for (unsigned channel = 0; channel < 4; ++channel)
                delta = std::max(delta, static_cast<unsigned>(std::abs(
                    static_cast<int>(actual.pixels[offset + channel]) - expected.pixels[offset + channel])));
            result.maxDelta = std::max(result.maxDelta, delta);
            if (delta > tolerance)
                ++result.differentPixels;
        }
        return result;
    }
}
