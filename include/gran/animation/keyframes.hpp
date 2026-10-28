#ifndef GRAN_ANIMATION_KEYFRAMES_HPP
#define GRAN_ANIMATION_KEYFRAMES_HPP

#include "gran/animation/visual_state.hpp"
#include "gran/style/style_sheet.hpp"

#include <cstdint>
#include <vector>

namespace gran::animation
{
    // Ключевой кадр в значениях: место, значения, заданные свойства (биты Animated), кривая участка до следующего
    // A keyframe in values: the place, the values, the set properties (Animated bits), the curve of the segment up to the next one
    struct FramePoint
    {
        float offset = 0.0f;
        VisualState values;
        std::uint32_t mask = 0;
        style::Easing easing;
    };

    /*
    ** Кадры одной анимации, готовые к выборке. base — значения стиля узла без анимации:
    ** свойство, которого нет в кадре на месте 0% (или 100%), берёт там базовое значение (как в CSS).
    ** mask — все свойства, заданные хотя бы одним кадром.
    **
    ** The frames of one animation, ready for sampling. base is the node style values without the animation:
    ** a property absent from a frame at 0% (or 100%) takes the base value there (as in CSS).
    ** mask is all the properties set by at least one frame.
    */
    struct FrameTrack
    {
        std::vector<FramePoint> points;
        VisualState base;
        style::Easing easing;
        std::uint32_t mask = 0;
    };

    // Ход анимации в момент времени / The animation course at a moment
    struct AnimationTiming
    {
        // Место на шкале кадров 0..1 с учётом направления повтора / The place on the frame scale 0..1 with the repeat direction applied
        float progress = 0.0f;
        // Значения анимации действуют (идёт, задержка с backwards, конец с forwards) / The animation values apply (running, a delay with backwards, the end with forwards)
        bool active = false;
        // Все повторы прошли / All the repeats have passed
        bool finished = false;
    };

    // ---- keyframe_track.cpp ----

    // Кадры набора поверх базового стиля; easing — кривая анимации для кадров без своей
    // The set frames over the base style; easing is the animation curve for frames without their own
    FrameTrack buildFrameTrack(const style::KeyframesRule& rule, const style::ComputedStyle& base, const style::Easing& easing);

    // ---- keyframe_timing.cpp ----

    // Ход анимации через elapsed секунд после её запуска (задержка — внутри) / The animation course elapsed seconds after its start (the delay is inside)
    AnimationTiming animationTiming(const style::AnimationSpec& spec, double elapsed) noexcept;

    // ---- keyframe_sample.cpp ----

    // Значение свойства на месте progress — между соседними кадрами, где оно задано / The property value at the place progress — between the neighbouring frames where it is set
    void sampleTrack(const FrameTrack& track, Animated property, float progress, VisualState& result);
}

#endif
