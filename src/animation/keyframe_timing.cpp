#include "gran/animation/keyframes.hpp"

#include <algorithm>
#include <cmath>

namespace gran::animation
{
    namespace
    {
        // Повтор с номером iteration идёт от конца к началу / The repeat with the number iteration runs from the end to the start
        bool reversed(style::AnimationDirection direction, double iteration) noexcept
        {
            bool odd = std::fmod(iteration, 2.0) >= 1.0;

            if (direction == style::AnimationDirection::REVERSE)
                return true;
            if (direction == style::AnimationDirection::ALTERNATE)
                return odd;
            if (direction == style::AnimationDirection::ALTERNATE_REVERSE)
                return !odd;
            return false;
        }

        // Место на шкале кадров для доли повтора local ∈ [0, 1] / The place on the frame scale for the repeat fraction local ∈ [0, 1]
        float directed(style::AnimationDirection direction, double iteration, double local) noexcept
        {
            if (reversed(direction, iteration))
                return static_cast<float>(1.0 - local);
            return static_cast<float>(local);
        }

        // Заполнение действует до начала (backwards) или после конца (forwards) / The fill acts before the start (backwards) or after the end (forwards)
        bool fills(style::AnimationFill fill, bool after) noexcept
        {
            if (fill == style::AnimationFill::BOTH)
                return true;
            if (after)
                return fill == style::AnimationFill::FORWARDS;
            return fill == style::AnimationFill::BACKWARDS;
        }
    }

    /*
    ** Как в Web Animations: до задержки — место начала первого повтора; после всех повторов —
    ** конец последнего (дробное число повторов, например 1.5, кончается на середине шкалы);
    ** в ходе — номер повтора и доля внутри него. Длительность 0 — анимация сразу в конце.
    **
    ** As in Web Animations: before the delay — the start place of the first repeat; after all the repeats —
    ** the end of the last one (a fractional repeat count, say 1.5, ends in the middle of the scale);
    ** while running — the repeat number and the fraction inside it. A zero duration — the animation is at the end at once.
    */
    AnimationTiming animationTiming(const style::AnimationSpec& spec, double elapsed) noexcept
    {
        AnimationTiming timing;
        double running = elapsed - spec.delay;
        double duration = std::max(1e-6, static_cast<double>(spec.duration));
        double total = static_cast<double>(spec.iterations) * duration;

        if (running < 0.0)
        {
            timing.active = fills(spec.fill, false);
            timing.progress = directed(spec.direction, 0.0, 0.0);
            return timing;
        }
        if (running >= total)
        {
            double last = std::max(0.0, std::ceil(static_cast<double>(spec.iterations)) - 1.0);
            timing.finished = true;
            timing.active = fills(spec.fill, true);
            timing.progress = directed(spec.direction, last, static_cast<double>(spec.iterations) - last);
            return timing;
        }
        double iteration = std::floor(running / duration);
        timing.active = true;
        timing.progress = directed(spec.direction, iteration, running / duration - iteration);
        return timing;
    }
}
