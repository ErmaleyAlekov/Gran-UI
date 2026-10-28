#include "gran/animation/internal/keyframe_runs.hpp"


namespace gran::animation::internal
{
    // Сжатие на месте: порядок оставшихся анимаций сохраняется / Compaction in place: the order of the remaining animations is kept
    void dropStaleRuns(std::vector<KeyframeRun>& runs, const ui::Node& node, const std::vector<style::AnimationSpec>& specs)
    {
        std::size_t kept = 0;

        for (std::size_t index = 0; index < runs.size(); ++index)
        {
            bool stale = false;
            if (runs[index].node == &node)
            {
                stale = true;
                for (const style::AnimationSpec& spec : specs)
                    stale = stale && spec.name != runs[index].spec.name;
            }
            if (stale)
                continue;
            if (kept != index)
                runs[kept] = std::move(runs[index]);
            ++kept;
        }
        runs.resize(kept);
    }

    // Линейный поиск: анимаций на окно — единицы / A linear search: there are a few animations per window
    KeyframeRun* findRun(std::vector<KeyframeRun>& runs, const ui::Node& node, const std::string& name) noexcept
    {
        for (KeyframeRun& run : runs)
        {
            if (run.node == &node && run.spec.name == name)
                return &run;
        }
        return nullptr;
    }

    /*
    ** Каждое свойство из маски кадров получает либо выборку на месте progress, либо базовое
    ** значение (задержка без backwards, конец без forwards): так анимация, закончившись,
    ** возвращает узлу его собственный стиль.
    **
    ** Every property of the frame mask gets either the sample at the place progress or the base
    ** value (a delay without backwards, the end without forwards): this way a finished animation
    ** gives the node back its own style.
    */
    void writeRun(const KeyframeRun& run, const AnimationTiming& timing, style::ComputedStyle& target)
    {
        VisualState current = run.track.base;

        for (std::size_t index = 0; index < ANIMATED_COUNT; ++index)
        {
            if ((run.track.mask & (1u << index)) == 0)
                continue;
            Animated property = static_cast<Animated>(index);
            if (timing.active)
                sampleTrack(run.track, property, timing.progress, current);
            applyVisual(target, current, property);
        }
    }

    /*
    ** Наследуемый цвет (color, accentColor) идёт вниз к потомкам, которые его не переопределили:
    ** иначе текст внутри кнопки остался бы с начальным цветом анимации кнопки.
    **
    ** An inherited color (color, accentColor) goes down to the descendants that did not override it:
    ** otherwise a text inside a button would keep the start color of the button animation.
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

    // Только цвета: остальные анимируемые свойства не наследуются / Only the colors: the other animatable properties are not inherited
    void spreadColors(ui::Node& node, std::uint32_t mask)
    {
        const std::uint32_t COLOR_BIT = 1u << static_cast<std::uint32_t>(Animated::COLOR);
        const std::uint32_t ACCENT_BIT = 1u << static_cast<std::uint32_t>(Animated::ACCENT_COLOR);

        if ((mask & (COLOR_BIT | ACCENT_BIT)) == 0)
            return;
        VisualState value = captureVisual(node.computed);
        if ((mask & COLOR_BIT) != 0)
            inheritDown(node, Animated::COLOR, value);
        if ((mask & ACCENT_BIT) != 0)
            inheritDown(node, Animated::ACCENT_COLOR, value);
    }
}
