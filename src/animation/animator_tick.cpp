#include "gran/animation/internal/keyframe_runs.hpp"

#include <algorithm>

namespace gran::animation
{
    namespace
    {
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
                    internal::inheritDown(*animation.node, property, current);
                if (progress >= 1.0f)
                    animation.mask &= ~(1u << index);
            }
            ui::markDirty(*animation.node, ui::DIRTY_PAINT);
        }

        // Удалить записи узла (переходы или анимации по кадрам) с сохранением порядка / Remove the node records (transitions or keyframe animations) keeping the order
        template <typename Record>
        void dropNode(std::vector<Record>& records, const ui::Node& node)
        {
            std::size_t kept = 0;

            for (std::size_t index = 0; index < records.size(); ++index)
            {
                if (records[index].node == &node)
                    continue;
                if (kept != index)
                    records[kept] = std::move(records[index]);
                ++kept;
            }
            records.resize(kept);
        }
    }

    // Все переходы на шаг (пустые удаляются), затем анимации по кадрам — их значения сильнее
    // All transitions one step (empty ones are removed), then keyframe animations — their values are stronger
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
        bool frames = tickKeyframes();
        return frames || !_animations.empty();
    }

    /*
    ** Закончившиеся анимации стоят (их значения с forwards кладёт в стиль syncKeyframes).
    ** Анимация скрытого узла (сам или предок visible="false": закрытый слой, другая вкладка) не
    ** шагает и не держит кадры: время идёт от запуска, поэтому при показе она сразу на своём месте.
    **
    ** Finished animations stand still (syncKeyframes puts their forwards values into the style).
    ** An animation of a hidden node (itself or an ancestor with visible="false": a closed layer, another tab) does not
    ** step and does not keep frames going: the time runs from the start, so when shown it is at its place at once.
    */
    bool Animator::tickKeyframes()
    {
        bool running = false;

        for (KeyframeRun& run : _runs)
        {
            if (run.finished || !ui::isShown(*run.node))
                continue;
            AnimationTiming timing = animationTiming(run.spec, _now - run.start);
            internal::writeRun(run, timing, run.node->computed);
            internal::spreadColors(*run.node, run.track.mask);
            ui::markDirty(*run.node, ui::DIRTY_PAINT);
            run.finished = timing.finished;
            running = running || !timing.finished;
        }
        return running;
    }

    // Анимации удаляемого узла снимаются: указатель на него станет недействительным
    // The animations of a node being removed are dropped: the pointer to it becomes invalid
    void Animator::forget(const ui::Node& node)
    {
        dropNode(_animations, node);
        dropNode(_runs, node);
    }
}
