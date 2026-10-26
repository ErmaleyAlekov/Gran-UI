#include "gran/animation/animator.hpp"
#include "gran/style/style_compare.hpp"

#include <cstring>

namespace gran::animation
{
    namespace
    {
        // Равенство цветов / Color equality
        bool sameColor(const Color& first, const Color& second) noexcept
        {
            return first.r == second.r && first.g == second.g && first.b == second.b && first.a == second.a;
        }

        // Равенство заливок: вид и цвет (градиенты сравниваются только видом — их переход мгновенный)
        // Fill equality: the kind and the color (gradients are compared by kind only — their transition is instant)
        bool samePaint(const style::Paint& first, const style::Paint& second) noexcept
        {
            return first.kind == second.kind && (first.kind != style::PaintKind::COLOR || sameColor(first.color, second.color));
        }

        // Равенство списков теней и преобразований поэлементно / The equality of shadow and transform lists element by element
        bool sameShadows(const std::vector<style::Shadow>& first, const std::vector<style::Shadow>& second) noexcept
        {
            if (first.size() != second.size())
                return false;
            for (std::size_t index = 0; index < first.size(); ++index)
            {
                const style::Shadow& a = first[index];
                const style::Shadow& b = second[index];
                if (a.offsetX != b.offsetX || a.offsetY != b.offsetY || a.blur != b.blur || a.spread != b.spread
                    || a.inset != b.inset || !sameColor(a.color, b.color))
                    return false;
            }
            return true;
        }
    }

    // Снимок анимируемой части стиля / A snapshot of the animatable part of the style
    VisualState captureVisual(const style::ComputedStyle& style)
    {
        return VisualState{style.background, style.borderColor, style.color, style.opacity, style.transform,
            style.shadows, style.borderRadius, style.accentColor};
    }

    // Изменение одного свойства / A change of one property
    bool visualChanged(const VisualState& first, const VisualState& second, Animated property) noexcept
    {
        switch (property)
        {
            case Animated::BACKGROUND: return !samePaint(first.background, second.background);
            case Animated::BORDER_COLOR: return !sameColor(first.borderColor, second.borderColor);
            case Animated::COLOR: return !sameColor(first.color, second.color);
            case Animated::OPACITY: return first.opacity != second.opacity;
            case Animated::BOX_SHADOW: return !sameShadows(first.shadows, second.shadows);
            case Animated::BORDER_RADIUS: return std::memcmp(&first.radius, &second.radius, sizeof(CornerRadii)) != 0;
            case Animated::ACCENT_COLOR: return !sameColor(first.accent, second.accent);
            default: break;
        }
        if (first.transform.size() != second.transform.size())
            return true;
        for (std::size_t index = 0; index < first.transform.size(); ++index)
        {
            const style::TransformStep& a = first.transform[index];
            const style::TransformStep& b = second.transform[index];
            if (a.kind != b.kind || a.x != b.x || a.y != b.y)
                return true;
        }
        return false;
    }
}
