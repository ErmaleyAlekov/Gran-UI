#ifndef GRAN_UTILS_COLOR_HPP
#define GRAN_UTILS_COLOR_HPP

#include <cstdint>

namespace gran
{
    /*
    ** Цвет sRGB с непредумноженной альфой, компоненты 0..1. Предумножение
    ** делается только при упаковке для GPU — так переходы между цветами
    ** не «темнеют» на прозрачных участках.
    **
    ** An sRGB color with a straight alpha, components 0..1. Premultiplication
    ** happens only when packing for the GPU — so transitions between colors
    ** do not "darken" on transparent parts.
    */
    struct Color
    {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 0.0f;
    };

    // Цвет из байтов 0..255 / A color from 0..255 bytes
    Color colorFromBytes(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) noexcept;

    // Упаковка для GPU: предумноженные RGBA8, R в младшем байте
    // Packing for the GPU: premultiplied RGBA8, R in the lowest byte
    std::uint32_t colorPackPremultiplied(const Color& color) noexcept;

    // Линейная смесь в sRGB с непредумноженной альфой (t от 0 до 1), как в переходах CSS
    // A linear mix in sRGB with a straight alpha (t from 0 to 1), as in CSS transitions
    Color colorMix(const Color& from, const Color& to, float t) noexcept;

    // Цвет с альфой, умноженной на непрозрачность / The color with the alpha multiplied by an opacity
    Color colorWithOpacity(const Color& color, float opacity) noexcept;

    // Цвет из HSL (оттенок в градусах, насыщенность и светлота 0..1)
    // A color from HSL (the hue in degrees, saturation and lightness 0..1)
    Color colorFromHsl(float hue, float saturation, float lightness, float alpha) noexcept;
}

#endif
