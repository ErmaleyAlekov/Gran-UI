#ifndef GRAN_ANIMATION_VISUAL_STATE_HPP
#define GRAN_ANIMATION_VISUAL_STATE_HPP

#include "gran/style/computed_style.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gran::animation
{
    // Анимируемые свойства (не влияют на компоновку — анимация не вызывает перекомпоновку)
    // The animatable properties (they do not affect layout — an animation causes no relayout)
    enum class Animated : std::uint8_t
    {
        BACKGROUND, BORDER_COLOR, COLOR, OPACITY, TRANSFORM, BOX_SHADOW, BORDER_RADIUS, ACCENT_COLOR, COUNT
    };

    // Число анимируемых свойств / The animatable property count
    constexpr std::size_t ANIMATED_COUNT = static_cast<std::size_t>(Animated::COUNT);

    // Значения анимируемых свойств / The animatable property values
    struct VisualState
    {
        style::Paint background;
        Color borderColor;
        Color color;
        float opacity = 1.0f;
        style::Transform transform;
        std::vector<style::Shadow> shadows;
        CornerRadii radius;
        Color accent;
    };

    // ---- easing.cpp ----

    // Значение кривой в точке t ∈ [0, 1] / The curve value at t ∈ [0, 1]
    float evaluateEasing(const style::Easing& easing, float t) noexcept;

    // ---- visual_state.cpp ----

    // Снимок анимируемых свойств / A snapshot of the animatable properties
    VisualState captureVisual(const style::ComputedStyle& style);

    // Изменилось ли свойство / Whether the property changed
    bool visualChanged(const VisualState& first, const VisualState& second, Animated property) noexcept;

    // ---- visual_apply.cpp ----

    // Запись одного свойства в стиль / Writing one property into the style
    void applyVisual(style::ComputedStyle& style, const VisualState& state, Animated property);

    // Номер свойства по имени из transition ("background", "boxShadow"...); COUNT — не анимируемое; all — особый случай
    // The property number by the transition name ("background", "boxShadow"...); COUNT — not animatable; all is a special case
    Animated animatedByName(const std::string& name) noexcept;

    // ---- interpolate.cpp ----

    // Промежуточное значение свойства при прогрессе k / The intermediate property value at progress k
    void interpolate(const VisualState& from, const VisualState& to, Animated property, float k, VisualState& result);
}

#endif
