#ifndef GRAN_STYLE_VALUES_HPP
#define GRAN_STYLE_VALUES_HPP

#include "gran/utils/color.hpp"
#include "gran/utils/geometry.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gran::style
{
    // Единица длины: auto, логические пиксели или проценты от родителя
    // A length unit: auto, logical pixels or percent of the parent
    enum class LengthUnit : std::uint8_t
    {
        AUTO,
        PIXELS,
        PERCENT
    };

    // Длина: "auto", "120", "50%" / A length: "auto", "120", "50%"
    struct Length
    {
        float value = 0.0f;
        LengthUnit unit = LengthUnit::AUTO;
    };

    // Граница дорожки сетки: пиксели, auto (по содержимому) или доля свободного места
    // A grid track bound: pixels, auto (by content) or a fraction of the free space
    enum class TrackBoundKind : std::uint8_t
    {
        PIXELS,
        AUTO,
        FRACTION
    };

    // Одна граница дорожки / One track bound
    struct TrackBound
    {
        TrackBoundKind kind = TrackBoundKind::AUTO;
        float value = 0.0f;
    };

    /*
    ** Дорожка сетки как minmax(min, max): "48" = minmax(48, 48), "auto" = minmax(auto, auto),
    ** "1fr" = minmax(auto, 1fr) — как в CSS Grid.
    **
    ** A grid track as minmax(min, max): "48" = minmax(48, 48), "auto" = minmax(auto, auto),
    ** "1fr" = minmax(auto, 1fr) — as in CSS Grid.
    */
    struct GridTrack
    {
        TrackBound min;
        TrackBound max;
    };

    // Тень: "x y размытие растяжение цвет [inset]" как box-shadow
    // A shadow: "x y blur spread color [inset]" as box-shadow
    struct Shadow
    {
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float blur = 0.0f;
        float spread = 0.0f;
        Color color;
        bool inset = false;
    };

    // Вид градиента / The gradient kind
    enum class GradientKind : std::uint8_t
    {
        LINEAR,
        RADIAL,
        CONIC
    };

    // Опорная точка градиента; position < 0 — «равномерно между соседями»
    // A gradient stop; position < 0 — "evenly between the neighbours"
    struct GradientStop
    {
        Color color;
        float position = -1.0f;
    };

    /*
    ** Градиент как в CSS: linear-gradient(угол, ...), radial-gradient([circle|ellipse] [at x y], ...),
    ** conic-gradient([from угол] [at x y], ...). Угол — в радианах (0 — вверх, по часовой),
    ** центр — доли размера блока.
    **
    ** A gradient as in CSS: linear-gradient(angle, ...), radial-gradient([circle|ellipse] [at x y], ...),
    ** conic-gradient([from angle] [at x y], ...). The angle is in radians (0 — up, clockwise),
    ** the center is fractions of the box size.
    */
    struct Gradient
    {
        GradientKind kind = GradientKind::LINEAR;
        float angle = 3.14159265f;
        bool circle = false;
        Vec2 center{0.5f, 0.5f};
        std::vector<GradientStop> stops;
    };

    // Заливка: ничего, сплошной цвет или градиент / A fill: nothing, a solid color or a gradient
    enum class PaintKind : std::uint8_t
    {
        NONE,
        COLOR,
        GRADIENT
    };

    // Значение свойств background, borderColor / The value of background, borderColor
    struct Paint
    {
        PaintKind kind = PaintKind::NONE;
        Color color;
        Gradient gradient;
    };

    // Вид кривой анимации / The animation curve kind
    enum class EasingKind : std::uint8_t
    {
        LINEAR,
        CUBIC_BEZIER,
        STEPS
    };

    // Кривая: linear, ease, ease-in, ease-out, ease-in-out, cubic-bezier(), steps()
    // A curve: linear, ease, ease-in, ease-out, ease-in-out, cubic-bezier(), steps()
    struct Easing
    {
        EasingKind kind = EasingKind::CUBIC_BEZIER;
        float x1 = 0.25f;
        float y1 = 0.1f;
        float x2 = 0.25f;
        float y2 = 1.0f;
        int steps = 1;
        bool jumpStart = false;
    };

    // Шаг преобразования / A transform step
    enum class TransformKind : std::uint8_t
    {
        TRANSLATE,
        SCALE,
        ROTATE,
        SKEW
    };

    // Одна функция преобразования: translate(x, y), scale(x, y), rotate(a), skew(x, y)
    // One transform function: translate(x, y), scale(x, y), rotate(a), skew(x, y)
    struct TransformStep
    {
        TransformKind kind = TransformKind::TRANSLATE;
        float x = 0.0f;
        float y = 0.0f;
    };

    // Цепочка преобразований слева направо, как в CSS
    // A chain of transforms from left to right, as in CSS
    using Transform = std::vector<TransformStep>;

    /*
    ** Переход: "background 0.2s ease 50ms". Имя свойства сверяется со списком
    ** анимируемых свойств при построении стиля ("all" — все анимируемые).
    **
    ** A transition: "background 0.2s ease 50ms". The property name is checked against
    ** the list of animatable properties when the style is built ("all" — all animatable ones).
    */
    struct TransitionSpec
    {
        std::string property;
        float duration = 0.0f;
        float delay = 0.0f;
        Easing easing;
    };

    // Направление повторов анимации (animation-direction) / The animation repeat direction (animation-direction)
    enum class AnimationDirection : std::uint8_t
    {
        NORMAL,
        REVERSE,
        ALTERNATE,
        ALTERNATE_REVERSE
    };

    // Значения до начала и после конца анимации (animation-fill-mode) / The values before the start and after the end of an animation (animation-fill-mode)
    enum class AnimationFill : std::uint8_t
    {
        NONE,
        FORWARDS,
        BACKWARDS,
        BOTH
    };

    /*
    ** Анимация по ключевым кадрам: "pulse 1.2s ease-in-out 0.2s infinite alternate both".
    ** Первое слово — имя набора <Keyframes>, первое время — длительность, второе — задержка;
    ** число или infinite — число повторов; прочие слова — направление, заполнение или кривая
    ** (кривая действует на каждом участке между кадрами, как animation-timing-function в CSS).
    **
    ** A keyframe animation: "pulse 1.2s ease-in-out 0.2s infinite alternate both".
    ** The first word is the <Keyframes> set name, the first time is the duration, the second is the delay;
    ** a number or infinite is the repeat count; the other words are the direction, the fill or the curve
    ** (the curve acts on every segment between frames, as animation-timing-function in CSS).
    */
    struct AnimationSpec
    {
        std::string name;
        float duration = 0.0f;
        float delay = 0.0f;
        Easing easing;
        // Число повторов; бесконечность — infinite / The repeat count; infinity — infinite
        float iterations = 1.0f;
        AnimationDirection direction = AnimationDirection::NORMAL;
        AnimationFill fill = AnimationFill::NONE;
    };
}

#endif
