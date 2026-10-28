#include "gran/animation/animator.hpp"

namespace gran::animation
{
    namespace
    {
        // Описание перехода для свойства: по имени или "all"; nullptr — перехода нет
        // The transition description for the property: by name or "all"; nullptr — no transition
        const style::TransitionSpec* findTransition(const std::vector<style::TransitionSpec>& transitions, Animated property)
        {
            for (const style::TransitionSpec& spec : transitions)
            {
                if (spec.property == "all" || animatedByName(spec.property) == property)
                    return &spec;
            }
            return nullptr;
        }

        // Анимация узла (новая, если её нет) / The node animation (a new one if there is none)
        NodeAnimation& animationOf(std::vector<NodeAnimation>& animations, ui::Node& node)
        {
            for (NodeAnimation& animation : animations)
            {
                if (animation.node == &node)
                    return animation;
            }
            animations.push_back(NodeAnimation{});
            animations.back().node = &node;
            return animations.back();
        }

        /*
        ** Ход свойства с начала: от показанного значения к целевому; остальные свойства анимации
        ** не трогаются (их ходы продолжаются без рывка). Копия свойства — интерполяция при k = 0.
        **
        ** A property course from the start: from the shown value to the target; the other animation properties
        ** are not touched (their courses continue without a jerk). A property copy is interpolation at k = 0.
        */
        void startTrack(NodeAnimation& animation, const VisualState& from, const VisualState& to, Animated property,
            const Track& track)
        {
            std::size_t index = static_cast<std::size_t>(property);

            interpolate(from, from, property, 0.0f, animation.from);
            interpolate(to, to, property, 0.0f, animation.to);
            animation.tracks[index] = track;
            animation.mask |= 1u << index;
        }
    }

    // Время кадра / The frame time
    void Animator::setTime(double now) noexcept
    {
        _now = now;
    }

    /*
    ** Каждое изменившееся свойство с переходом начинает ход; без перехода — прыжок к цели (и остановка хода).
    ** Свойства, которые ведёт анимация по кадрам, переходами не трогаются: их значение уже в next.
    **
    ** Every changed property with a transition starts a course; without a transition — a jump to the target (and the course stops).
    ** The properties driven by a keyframe animation are not touched by transitions: their value is already in next.
    */
    void Animator::styleChanged(ui::Node& node, const style::ComputedStyle& shown, style::ComputedStyle& next)
    {
        VisualState from = captureVisual(shown);
        VisualState to = captureVisual(next);
        std::uint32_t driven = syncKeyframes(node, next);

        for (std::size_t index = 0; index < ANIMATED_COUNT; ++index)
        {
            Animated property = static_cast<Animated>(index);
            if (!visualChanged(from, to, property) || (driven & (1u << index)) != 0)
                continue;
            const style::TransitionSpec* spec = findTransition(next.transitions, property);
            if (spec == nullptr || spec->duration <= 0.0f)
            {
                animationOf(_animations, node).mask &= ~(1u << index);
                continue;
            }
            startTrack(animationOf(_animations, node), from, to, property,
                Track{_now, spec->delay, spec->duration, spec->easing});
            applyVisual(next, from, property);
        }
    }
}
