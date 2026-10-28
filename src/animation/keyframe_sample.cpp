#include "gran/animation/keyframes.hpp"

namespace gran::animation
{
    namespace
    {
        /*
        ** Опорная точка участка: кадр, где свойство задано, или неявный кадр с базовым значением
        ** на краю шкалы (0% — с кривой анимации, 100% — кривая уже не нужна).
        **
        ** A segment anchor: a frame where the property is set, or an implicit frame with the base value
        ** at the scale edge (0% — with the animation curve, 100% — the curve is not needed anymore).
        */
        struct Anchor
        {
            float offset;
            const VisualState* values;
            const style::Easing* easing;
        };

        // Последний кадр со свойством на месте не дальше progress / The last frame with the property at a place not beyond progress
        Anchor anchorBefore(const FrameTrack& track, std::uint32_t bit, float progress) noexcept
        {
            Anchor anchor{0.0f, &track.base, &track.easing};

            for (const FramePoint& point : track.points)
            {
                if (point.offset > progress)
                    break;
                if ((point.mask & bit) != 0)
                    anchor = Anchor{point.offset, &point.values, &point.easing};
            }
            return anchor;
        }

        // Первый кадр со свойством дальше места начала участка (и не раньше progress) / The first frame with the property past the segment start (and not before progress)
        Anchor anchorAfter(const FrameTrack& track, std::uint32_t bit, float progress, float start) noexcept
        {
            for (const FramePoint& point : track.points)
            {
                if ((point.mask & bit) != 0 && point.offset >= progress && point.offset > start)
                    return Anchor{point.offset, &point.values, &point.easing};
            }
            return Anchor{1.0f, &track.base, &track.easing};
        }
    }

    /*
    ** Участок [до, после] вокруг progress; доля внутри участка идёт через кривую начального кадра.
    ** Совпавшие места (кадр ровно на progress или кадр 100% с прогрессом 1) дают значение кадра.
    **
    ** The segment [before, after] around progress; the fraction inside the segment goes through the start frame curve.
    ** Equal places (a frame exactly at progress or a 100% frame with progress 1) give the frame value.
    */
    void sampleTrack(const FrameTrack& track, Animated property, float progress, VisualState& result)
    {
        std::uint32_t bit = 1u << static_cast<std::uint32_t>(property);
        Anchor before = anchorBefore(track, bit, progress);
        Anchor after = anchorAfter(track, bit, progress, before.offset);
        float span = after.offset - before.offset;
        float local = 1.0f;

        if (before.offset >= progress && before.offset > 0.0f)
            local = 0.0f;
        else if (span > 1e-6f)
            local = (progress - before.offset) / span;
        interpolate(*before.values, *after.values, property, evaluateEasing(*before.easing, local), result);
    }
}
