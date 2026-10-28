#include "gran/animation/internal/keyframe_runs.hpp"

namespace gran::animation
{
    namespace
    {
        /*
        ** Новые имена запускаются с текущего кадра; у идущих обновляются описание и кадры:
        ** база (стиль без анимации) могла смениться вместе с состоянием узла (hover и т. п.).
        **
        ** New names start from the current frame; the running ones get their description and frames updated:
        ** the base (the style without the animation) could change together with the node state (hover and so on).
        */
        void startRuns(std::vector<KeyframeRun>& runs, const style::StyleSheet& sheet, ui::Node& node,
            const style::ComputedStyle& base, double now)
        {
            for (const style::AnimationSpec& spec : base.animations)
            {
                const style::KeyframesRule* rule = sheet.findKeyframes(spec.name);
                if (rule == nullptr)
                    continue;
                KeyframeRun* run = internal::findRun(runs, node, spec.name);
                if (run == nullptr)
                {
                    runs.push_back(KeyframeRun{});
                    run = &runs.back();
                    run->node = &node;
                    run->start = now;
                }
                run->spec = spec;
                run->track = buildFrameTrack(*rule, base, spec.easing);
            }
        }

        // Значения действующих анимаций в стиль — в порядке списка, поздняя сильнее / The active animation values into the style — in the list order, a later one is stronger
        std::uint32_t applyRuns(std::vector<KeyframeRun>& runs, ui::Node& node, style::ComputedStyle& next, double now)
        {
            std::uint32_t driven = 0;

            for (const style::AnimationSpec& spec : next.animations)
            {
                KeyframeRun* run = internal::findRun(runs, node, spec.name);
                if (run == nullptr)
                    continue;
                AnimationTiming timing = animationTiming(run->spec, now - run->start);
                if (!timing.active)
                    continue;
                internal::writeRun(*run, timing, next);
                driven |= run->track.mask;
            }
            return driven;
        }
    }

    // Наборы кадров документа / The document keyframe sets
    void Animator::setStyleSheet(const style::StyleSheet* sheet) noexcept
    {
        _sheet = sheet;
    }

    // При загрузке переходам нечего делать — только запуск анимаций по кадрам / On load transitions have nothing to do — only starting keyframe animations
    void Animator::styleCreated(ui::Node& node, style::ComputedStyle& next)
    {
        syncKeyframes(node, next);
    }

    /*
    ** Кадры строятся по чистому next (до записи значений анимаций), затем значения ложатся
    ** в next: узел сразу показывает анимацию, а не свой стиль до следующего шага.
    **
    ** The frames are built from the clean next (before writing the animation values), then the values go
    ** into next: the node shows the animation at once and not its own style until the next step.
    */
    std::uint32_t Animator::syncKeyframes(ui::Node& node, style::ComputedStyle& next)
    {
        internal::dropStaleRuns(_runs, node, next.animations);
        if (_sheet == nullptr || next.animations.empty())
            return 0;
        startRuns(_runs, *_sheet, node, next, _now);
        return applyRuns(_runs, node, next, _now);
    }
}
