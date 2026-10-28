#include "gran/animation/visual_state.hpp"

namespace gran::animation
{
    // Одно свойство из снимка в стиль / One property from the snapshot into the style
    void applyVisual(style::ComputedStyle& style, const VisualState& state, Animated property)
    {
        switch (property)
        {
            case Animated::BACKGROUND: style.background = state.background; break;
            case Animated::BORDER_COLOR: style.borderColor = state.borderColor; break;
            case Animated::COLOR: style.color = state.color; break;
            case Animated::OPACITY: style.opacity = state.opacity; break;
            case Animated::TRANSFORM: style.transform = state.transform; break;
            case Animated::BOX_SHADOW: style.shadows = state.shadows; break;
            case Animated::BORDER_RADIUS: style.borderRadius = state.radius; break;
            default: style.accentColor = state.accent; break;
        }
    }

    // Имена — как атрибуты разметки; "all" обрабатывает аниматор / The names are as the markup attributes; "all" is handled by the animator
    Animated animatedByName(const std::string& name) noexcept
    {
        static const struct { const char* name; Animated property; } NAMES[] = {
            {"background", Animated::BACKGROUND}, {"borderColor", Animated::BORDER_COLOR}, {"color", Animated::COLOR},
            {"opacity", Animated::OPACITY}, {"transform", Animated::TRANSFORM}, {"boxShadow", Animated::BOX_SHADOW},
            {"borderRadius", Animated::BORDER_RADIUS}, {"accentColor", Animated::ACCENT_COLOR}};

        for (const auto& entry : NAMES)
        {
            if (name == entry.name)
                return entry.property;
        }
        return Animated::COUNT;
    }
}
