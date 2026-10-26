#include "gran/animation/animator.hpp"

#include <algorithm>

namespace gran::animation
{
    namespace
    {
        /*
        ** Наследуемый цвет (color, accentColor) идёт вниз к потомкам, которые его не переопределили:
        ** иначе текст внутри кнопки остался бы с начальным цветом перехода кнопки.
        **
        ** An inherited color (color, accentColor) goes down to the descendants that did not override it:
        ** otherwise a text inside a button would keep the start color of the button transition.
        */
        void inheritDown(ui::Node& node, Animated property, const VisualState& value)
        {
            std::uint8_t bit = style::INHERITED_ACCENT;

            if (property == Animated::COLOR)
                bit = style::INHERITED_COLOR;
            for (const std::unique_ptr<ui::Node>& child : node.children)
            {
                if ((child->computed.inheritedColors & bit) == 0)
                    continue;
                applyVisual(child->computed, value, property);
                ui::markDirty(*child, ui::DIRTY_PAINT);
                inheritDown(*child, property, value);
            }
        }

        /*
        ** Шаг одной анимации: каждое идущее свойство получает значение кривой; во время задержки
        ** держится начальное. Закончившиеся свойства снимаются с маски. Узел перерисовывается.
        **
        ** A step of one animation: every running property gets the curve value; during the delay
        ** the start value is held. The finished properties are removed from the mask. The node is repainted.
        */
        void advance(NodeAnimation& animation, double now)
        {
            VisualState current = captureVisual(animation.node->computed);

            for (std::size_t index = 0; index < ANIMATED_COUNT; ++index)
            {
                if ((animation.mask & (1u << index)) == 0)
                    continue;
                const Track& track = animation.tracks[index];
                double elapsed = now - track.start - track.delay;
                float progress = static_cast<float>(std::max(0.0, elapsed) / std::max(1e-6f, track.duration));
                Animated property = static_cast<Animated>(index);
                interpolate(animation.from, animation.to, property, evaluateEasing(track.easing, progress), current);
                applyVisual(animation.node->computed, current, property);
                if (property == Animated::COLOR || property == Animated::ACCENT_COLOR)
                    inheritDown(*animation.node, property, current);
                if (progress >= 1.0f)
                    animation.mask &= ~(1u << index);
            }
            ui::markDirty(*animation.node, ui::DIRTY_PAINT);
        }
    }

    // Все анимации на шаг; пустые удаляются / All animations one step; empty ones are removed
    bool Animator::tick()
    {
        std::size_t kept = 0;

        for (std::size_t index = 0; index < _animations.size(); ++index)
        {
            if (_animations[index].mask != 0)
                advance(_animations[index], _now);
            if (_animations[index].mask == 0)
                continue;
            if (kept != index)
                _animations[kept] = std::move(_animations[index]);
            ++kept;
        }
        _animations.resize(kept);
        return !_animations.empty();
    }

    // Анимации удаляемого узла снимаются: указатель на него станет недействительным
    // The animations of a node being removed are dropped: the pointer to it becomes invalid
    void Animator::forget(const ui::Node& node)
    {
        std::size_t kept = 0;

        for (std::size_t index = 0; index < _animations.size(); ++index)
        {
            if (_animations[index].node == &node)
                continue;
            if (kept != index)
                _animations[kept] = std::move(_animations[index]);
            ++kept;
        }
        _animations.resize(kept);
    }
}
