#include "gran/utils/color.hpp"

#include <algorithm>
#include <cmath>

namespace gran
{
    namespace
    {
        // Компонента 0..1 → байт с округлением / A 0..1 component → a byte with rounding
        std::uint32_t toByte(float value) noexcept
        {
            float clamped = std::min(1.0f, std::max(0.0f, value));
            return static_cast<std::uint32_t>(std::lround(clamped * 255.0f));
        }

        // Одна компонента HSL по формуле CSS Color 4 / One HSL component by the CSS Color 4 formula
        float hslChannel(float n, float hue, float saturation, float lightness) noexcept
        {
            float k = std::fmod(n + hue / 30.0f, 12.0f);
            float amplitude = saturation * std::min(lightness, 1.0f - lightness);
            float wave = std::max(-1.0f, std::min(std::min(k - 3.0f, 9.0f - k), 1.0f));
            return lightness - amplitude * wave;
        }
    }

    // Байты делятся на 255 / The bytes are divided by 255
    Color colorFromBytes(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) noexcept
    {
        return Color{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    }

    // Предумножение и упаковка RGBA8 (R — младший байт, как DXGI_FORMAT_R8G8B8A8_UNORM)
    // Premultiplication and RGBA8 packing (R is the lowest byte, as DXGI_FORMAT_R8G8B8A8_UNORM)
    std::uint32_t colorPackPremultiplied(const Color& color) noexcept
    {
        float alpha = std::min(1.0f, std::max(0.0f, color.a));

        return toByte(color.r * alpha) | (toByte(color.g * alpha) << 8)
            | (toByte(color.b * alpha) << 16) | (toByte(alpha) << 24);
    }

    // Смесь покомпонентно; t за пределами 0..1 ограничивается
    // A per-component mix; t outside 0..1 is clamped
    Color colorMix(const Color& from, const Color& to, float t) noexcept
    {
        float k = std::min(1.0f, std::max(0.0f, t));

        return Color{from.r + (to.r - from.r) * k, from.g + (to.g - from.g) * k,
            from.b + (to.b - from.b) * k, from.a + (to.a - from.a) * k};
    }

    // Непрозрачность умножает только альфу / The opacity multiplies only the alpha
    Color colorWithOpacity(const Color& color, float opacity) noexcept
    {
        return Color{color.r, color.g, color.b, color.a * opacity};
    }

    // Оттенок приводится к 0..360, остальные компоненты ограничиваются 0..1
    // The hue is brought into 0..360, the other components are clamped to 0..1
    Color colorFromHsl(float hue, float saturation, float lightness, float alpha) noexcept
    {
        float h = std::fmod(hue, 360.0f);
        float s = std::min(1.0f, std::max(0.0f, saturation));
        float l = std::min(1.0f, std::max(0.0f, lightness));

        if (h < 0.0f)
            h += 360.0f;
        return Color{hslChannel(0.0f, h, s, l), hslChannel(8.0f, h, s, l),
            hslChannel(4.0f, h, s, l), std::min(1.0f, std::max(0.0f, alpha))};
    }
}
