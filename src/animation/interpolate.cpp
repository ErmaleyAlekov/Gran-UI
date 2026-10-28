#include "gran/animation/visual_state.hpp"
#include "gran/animation/internal/interpolation.hpp"

namespace gran::animation
{
    namespace
    {
        // Радиусы углов / The corner radii
        CornerRadii mixRadii(const CornerRadii& from, const CornerRadii& to, float k) noexcept
        {
            return CornerRadii{internal::mixNumber(from.topLeft, to.topLeft, k), internal::mixNumber(from.topRight, to.topRight, k),
                internal::mixNumber(from.bottomRight, to.bottomRight, k), internal::mixNumber(from.bottomLeft, to.bottomLeft, k)};
        }
    }

    // Промежуточное значение одного свойства / The intermediate value of one property
    void interpolate(const VisualState& from, const VisualState& to, Animated property, float k, VisualState& result)
    {
        switch (property)
        {
            case Animated::BACKGROUND: result.background = internal::mixPaint(from.background, to.background, k); break;
            case Animated::BORDER_COLOR: result.borderColor = internal::mixColor(from.borderColor, to.borderColor, k); break;
            case Animated::COLOR: result.color = internal::mixColor(from.color, to.color, k); break;
            case Animated::OPACITY: result.opacity = internal::mixNumber(from.opacity, to.opacity, k); break;
            case Animated::TRANSFORM: result.transform = internal::mixTransform(from.transform, to.transform, k); break;
            case Animated::BOX_SHADOW: result.shadows = internal::mixShadows(from.shadows, to.shadows, k); break;
            case Animated::BORDER_RADIUS: result.radius = mixRadii(from.radius, to.radius, k); break;
            default: result.accent = internal::mixColor(from.accent, to.accent, k); break;
        }
    }
}
