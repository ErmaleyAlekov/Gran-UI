#ifndef GRAN_ANIMATION_INTERNAL_INTERPOLATION_HPP
#define GRAN_ANIMATION_INTERNAL_INTERPOLATION_HPP

#include "gran/style/values.hpp"

#include <vector>

namespace gran::animation::internal
{
    // ---- interpolate_values.cpp ----

    // Число между a и b при прогрессе k / A number between a and b at progress k
    float mixNumber(float a, float b, float k) noexcept;

    // Цвет в предумноженном пространстве (CSS Color 4) / A color in premultiplied space (CSS Color 4)
    Color mixColor(const Color& from, const Color& to, float k) noexcept;

    // Преобразования одной формы — по шагам, иначе значение конца / Transforms of one shape — step by step, otherwise the end value
    style::Transform mixTransform(const style::Transform& from, const style::Transform& to, float k);

    // ---- interpolate_shadows.cpp ----

    // Тени попарно, недостающие — прозрачные / Shadows pairwise, the missing ones are transparent
    std::vector<style::Shadow> mixShadows(const std::vector<style::Shadow>& from, const std::vector<style::Shadow>& to, float k);

    // Заливка: цвет ↔ цвет (и «нет» как прозрачный цвет), градиенты — значение конца
    // A fill: color ↔ color (and "none" as a transparent color), gradients — the end value
    style::Paint mixPaint(const style::Paint& from, const style::Paint& to, float k);
}

#endif
