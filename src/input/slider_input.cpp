#include "gran/input/input_router.hpp"

#include <algorithm>
#include <cmath>

namespace gran::input
{
    /*
    ** Новое значение: округление к шагу от минимума, ограничение диапазоном; при изменении —
    ** перерисовка и onChange.
    **
    ** The new value: rounding to the step from the minimum, clamping by the range; on a change —
    ** a repaint and onChange.
    */
    static bool assignSliderValue(RouterState& state, ui::Node& slider, double value)
    {
        ui::NodeContent& content = slider.content;

        if (content.step > 0.0f)
        {
            const double step = content.step;
            double remainder = std::fmod(std::fmod(value, step) - std::fmod(content.minimum, step), step);
            if (remainder >= step * 0.5)
                remainder -= step;
            else if (remainder < -step * 0.5)
                remainder += step;
            value -= remainder;
        }
        const auto clamped = static_cast<float>(std::min(static_cast<double>(content.maximum),
            std::max(static_cast<double>(content.minimum), value)));
        if (clamped == content.value)
            return false;
        content.value = clamped;
        ui::markDirty(slider, ui::DIRTY_PAINT);
        if (!content.onChange.empty())
            state.actions->action(content.onChange, slider);
        return true;
    }

    bool setSliderValue(RouterState& state, ui::Node& slider, float value)
    {
        return assignSliderValue(state, slider, value);
    }

    // Та же геометрия, что у отрисовки: центр бегунка ходит от d/2 до ширины − d/2
    // The same geometry as the rendering: the thumb center moves from d/2 to width − d/2
    void dragSlider(RouterState& state, ui::Node& slider, float x)
    {
        const style::ComputedStyle& style = slider.computed;
        const Rect& frame = slider.layout.frame;
        float left = frame.x + style.padding.left + style.borderWidth.left;
        float width = frame.width - style.padding.left - style.padding.right - style.borderWidth.left - style.borderWidth.right;
        float thumb = std::round(style.fontSize * 1.15f);
        float travel = std::max(1.0f, width - thumb);
        float t = std::min(1.0f, std::max(0.0f, (x - left - thumb * 0.5f) / travel));

        setSliderValue(state, slider, static_cast<float>((1.0 - t) * slider.content.minimum
            + static_cast<double>(t) * slider.content.maximum));
    }

    // Стрелки: шаг или сотая доля диапазона / The arrows: the step or a hundredth of the range
    bool stepSlider(RouterState& state, ui::Node& slider, int direction)
    {
        double step = slider.content.step;

        if (step <= 0.0f)
            step = (static_cast<double>(slider.content.maximum) - slider.content.minimum) / 100.0;
        const double next = slider.content.value + step * direction;
        return assignSliderValue(state, slider, next);
    }
}
