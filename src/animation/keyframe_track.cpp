#include "gran/animation/keyframes.hpp"

namespace gran::animation
{
    namespace
    {
        /*
        ** Кадр → значения: объявления кадра накладываются на копию базового стиля, поэтому
        ** незаданные свойства кадра равны базовым, а маска говорит, какие заданы на самом деле.
        **
        ** A frame → values: the frame declarations are laid over a copy of the base style, so
        ** the unset frame properties equal the base ones, and the mask tells which are really set.
        */
        FramePoint pointOf(const style::Keyframe& frame, const style::ComputedStyle& base, const style::Easing& easing)
        {
            style::ComputedStyle styled = base;
            FramePoint point;

            for (const style::Declaration& declaration : frame.values)
            {
                style::applyDeclaration(styled, declaration);
                Animated property = animatedByName(style::propertyInfo(declaration.first).name);
                if (property != Animated::COUNT)
                    point.mask |= 1u << static_cast<std::uint32_t>(property);
            }
            point.offset = frame.offset;
            point.values = captureVisual(styled);
            point.easing = easing;
            if (frame.hasEasing)
                point.easing = frame.easing;
            return point;
        }
    }

    // Базовые значения снимаются один раз, кадры — по порядку набора / The base values are captured once, the frames — in the set order
    FrameTrack buildFrameTrack(const style::KeyframesRule& rule, const style::ComputedStyle& base, const style::Easing& easing)
    {
        FrameTrack track;

        track.base = captureVisual(base);
        track.easing = easing;
        track.points.reserve(rule.frames.size());
        for (const style::Keyframe& frame : rule.frames)
        {
            track.points.push_back(pointOf(frame, base, easing));
            track.mask |= track.points.back().mask;
        }
        return track;
    }
}
