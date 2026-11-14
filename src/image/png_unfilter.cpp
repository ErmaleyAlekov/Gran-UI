#include "gran/image/internal/png_internal.hpp"

#include <cstdlib>

namespace gran::image::internal
{
    namespace
    {
        // Предсказание фильтра: Sub — левый, Up — верхний, Average — среднее, Paeth — ближайший к left + up − upLeft
        // The filter prediction: Sub — the left, Up — the upper, Average — the mean, Paeth — the nearest to left + up − upLeft
        int predict(std::uint8_t filter, int left, int up, int upLeft) noexcept
        {
            if (filter == 1)
                return left;
            if (filter == 2)
                return up;
            if (filter == 3)
                return (left + up) / 2;
            int estimate = left + up - upLeft;
            int toLeft = std::abs(estimate - left);
            int toUp = std::abs(estimate - up);
            int toUpLeft = std::abs(estimate - upLeft);
            if (toLeft <= toUp && toLeft <= toUpLeft)
                return left;
            if (toUp <= toUpLeft)
                return up;
            return upLeft;
        }

        /*
        ** Одна строка: к каждому байту прибавляется предсказание по фильтру (Sub — левый байт
        ** того же канала, Up — верхний, Average — их среднее, Paeth). Первая строка «сверху» — нули.
        **
        ** One row: each byte gets the prediction by the filter added (Sub — the left byte
        ** of the same channel, Up — the upper one, Average — their mean, Paeth). The first row's "upper" is zeros.
        */
        bool unfilterRow(std::uint8_t* row, const std::uint8_t* previous, std::size_t length, std::size_t step,
            std::uint8_t filter)
        {
            if (filter > 4)
                return false;
            for (std::size_t index = 0; index < length && filter != 0; ++index)
            {
                int left = 0;
                int up = 0;
                int upLeft = 0;
                if (index >= step)
                    left = row[index - step];
                if (previous != nullptr)
                    up = previous[index];
                if (previous != nullptr && index >= step)
                    upLeft = previous[index - step];
                row[index] = static_cast<std::uint8_t>(row[index] + predict(filter, left, up, upLeft));
            }
            return true;
        }
    }

    // Серый 1, RGB 3, палитра 1, серый+альфа 2, RGBA 4 / Gray 1, RGB 3, palette 1, gray+alpha 2, RGBA 4
    std::size_t channelsOf(std::uint8_t colorType) noexcept
    {
        static const std::size_t CHANNELS[7] = {1, 0, 3, 1, 2, 0, 4};

        if (colorType > 6)
            return 0;
        return CHANNELS[colorType];
    }

    // Биты строки, округлённые до байта / The row bits rounded up to a byte
    std::size_t rowBytes(const PngHeader& header) noexcept
    {
        return (static_cast<std::size_t>(header.width) * channelsOf(header.colorType) * header.depth + 7) / 8;
    }

    // Все строки по порядку; шаг предсказания — байт в пикселе (не меньше 1) / All rows in order; the prediction step is bytes per pixel (at least 1)
    bool unfilter(std::vector<std::uint8_t>& raw, const PngHeader& header)
    {
        std::size_t length = rowBytes(header);
        std::size_t stride = length + 1;
        std::size_t step = (channelsOf(header.colorType) * header.depth + 7) / 8;

        // Произведение в 64 битах: в 32-битной сборке stride × height переполнило бы size_t и могло совпасть
        // с маленьким raw.size()
        // The product in 64 bits: in a 32-bit build stride × height would overflow size_t and could match
        // a small raw.size()
        if (static_cast<std::uint64_t>(raw.size()) != static_cast<std::uint64_t>(stride) * header.height)
            return false;
        for (std::size_t y = 0; y < header.height; ++y)
        {
            std::uint8_t* row = raw.data() + y * stride;
            const std::uint8_t* previous = nullptr;
            if (y > 0)
                previous = row - stride + 1;
            if (!unfilterRow(row + 1, previous, length, step, row[0]))
                return false;
        }
        return true;
    }
}
