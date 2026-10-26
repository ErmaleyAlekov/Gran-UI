#ifndef GRAN_ANIMATION_ANIMATOR_HPP
#define GRAN_ANIMATION_ANIMATOR_HPP

#include "gran/ui/style_pass.hpp"

#include <cstdint>
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

    // Ход одного свойства: начало, задержка, длительность, кривая / The course of one property: the start, the delay, the duration, the curve
    struct Track
    {
        double start = 0.0;
        float delay = 0.0f;
        float duration = 0.0f;
        style::Easing easing;
    };

    // Анимация узла: откуда, куда и ходы свойств (бит маски — свойство идёт) / A node animation: from, to and the property tracks (a mask bit — the property runs)
    struct NodeAnimation
    {
        ui::Node* node = nullptr;
        VisualState from;
        VisualState to;
        Track tracks[ANIMATED_COUNT];
        std::uint32_t mask = 0;
    };

    /*
    ** Переходы CSS (transition). При пересчёте стиля узла (смена состояния hover/pressed...)
    ** изменившиеся анимируемые свойства с описанным переходом не прыгают к новому значению,
    ** а плавно идут от текущего показанного. Пересчёт посреди анимации начинает новую
    ** от текущей точки — без рывка (как в браузерах).
    **
    ** CSS transitions. When a node style is recomputed (a hover/pressed... state change)
    ** the changed animatable properties with a described transition do not jump to the new value,
    ** they move smoothly from the currently shown one. A recompute in the middle of an animation starts
    ** a new one from the current point — without a jerk (as in browsers).
    */
    class Animator : public ui::StyleObserver
    {
    public:
        // ---- animator.cpp ----

        // Время текущего кадра (секунды) / The current frame time (seconds)
        void setTime(double now) noexcept;

        // Начать переходы для изменившихся свойств / Start transitions for the changed properties
        void styleChanged(ui::Node& node, const style::ComputedStyle& shown, style::ComputedStyle& next) override;

        // Шаг анимаций; true — ещё идут (нужен следующий кадр) / An animation step; true — still running (the next frame is needed)
        bool tick();

        // Узел удаляется из дерева / The node is removed from the tree
        void forget(const ui::Node& node);

    private:
        std::vector<NodeAnimation> _animations;
        double _now = 0.0;
    };

    // ---- easing.cpp ----

    // Значение кривой в точке t ∈ [0, 1] / The curve value at t ∈ [0, 1]
    float evaluateEasing(const style::Easing& easing, float t) noexcept;

    // ---- visual_state.cpp ----

    // Снимок анимируемых свойств / A snapshot of the animatable properties
    VisualState captureVisual(const style::ComputedStyle& style);

    // Запись одного свойства в стиль / Writing one property into the style
    void applyVisual(style::ComputedStyle& style, const VisualState& state, Animated property);

    // Изменилось ли свойство / Whether the property changed
    bool visualChanged(const VisualState& first, const VisualState& second, Animated property) noexcept;

    // Номер свойства по имени из transition ("background", "boxShadow"...); COUNT — не анимируемое; all — особый случай
    // The property number by the transition name ("background", "boxShadow"...); COUNT — not animatable; all is a special case
    Animated animatedByName(const std::string& name) noexcept;

    // ---- interpolate.cpp ----

    // Промежуточное значение свойства при прогрессе k / The intermediate property value at progress k
    void interpolate(const VisualState& from, const VisualState& to, Animated property, float k, VisualState& result);
}

#endif
