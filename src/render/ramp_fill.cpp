#include "gran/render/gradient_ramps.hpp"
#include "gran/render/gpu_types.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render
{
    namespace
    {
        // Наибольшее число строк рампы / The largest ramp row count
        constexpr std::uint32_t MAX_ROWS = 4096;

        // Цвет с предумноженной альфой / A color with premultiplied alpha
        Color premultiply(const Color& color) noexcept
        {
            return Color{color.r * color.a, color.g * color.a, color.b * color.a, color.a};
        }

        // Цвет в точке t: между соседними опорными точками, за краями — крайние цвета
        // The color at t: between the neighbouring stops, beyond the edges — the edge colors
        Color colorAt(const std::vector<style::GradientStop>& stops, float t) noexcept
        {
            if (t <= stops.front().position)
                return premultiply(stops.front().color);
            for (std::size_t index = 1; index < stops.size(); ++index)
            {
                const style::GradientStop& left = stops[index - 1];
                const style::GradientStop& right = stops[index];
                if (t > right.position)
                    continue;
                float span = right.position - left.position;
                float k = 0.0f;
                if (span > 1e-6f)
                    k = (t - left.position) / span;
                return colorMix(premultiply(left.color), premultiply(right.color), k);
            }
            return premultiply(stops.back().color);
        }

        // Компонента 0..1 → байт / A 0..1 component → a byte
        std::uint8_t toByte(float value) noexcept
        {
            return static_cast<std::uint8_t>(std::lround(std::min(1.0f, std::max(0.0f, value)) * 255.0f));
        }
    }

    // Поколение рамп / The ramp generation
    std::uint32_t GradientRamps::generation() const noexcept
    {
        return _generation;
    }

    // Тексель i — середина интервала: t = (i + 0.5) / 256 / Texel i is the interval middle: t = (i + 0.5) / 256
    void GradientRamps::fillRow(std::uint32_t row, const style::Gradient& gradient)
    {
        std::uint8_t* target = _pixels.data() + static_cast<std::size_t>(row) * RAMP_WIDTH * 4;

        if (gradient.stops.empty())
            return;
        for (std::uint32_t texel = 0; texel < RAMP_WIDTH; ++texel)
        {
            Color color = colorAt(gradient.stops, (static_cast<float>(texel) + 0.5f) / RAMP_WIDTH);
            target[texel * 4] = toByte(color.r);
            target[texel * 4 + 1] = toByte(color.g);
            target[texel * 4 + 2] = toByte(color.b);
            target[texel * 4 + 3] = toByte(color.a);
        }
    }

    // Рост вдвое с сохранением строк; за пределом — сброс всех строк / Doubling keeping the rows; beyond the limit — resetting all rows
    void GradientRamps::growOrReset()
    {
        _generation += 1;
        _dirtyFirst = 0;
        if (_capacity * 2 > MAX_ROWS)
        {
            _rows.clear();
            _used = 0;
            _dirtyLast = 0;
            return;
        }
        _capacity *= 2;
        _pixels.resize(static_cast<std::size_t>(_capacity) * RAMP_WIDTH * 4, 0);
        _dirtyLast = _used;
    }
}
