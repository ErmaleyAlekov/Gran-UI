#include "gran/input/input_router.hpp"

#include <algorithm>
#include <cmath>

namespace gran::input
{
    namespace
    {
        /*
        ** Новое значение: округление к шагу от минимума, ограничение диапазоном; при изменении —
        ** перерисовка и onChange.
        **
        ** The new value: rounding to the step from the minimum, clamping by the range; on a change —
        ** a repaint and onChange.
        */
        bool setValue(RouterState& state, ui::Node& slider, float value)
        {
            ui::NodeContent& content = slider.content;

            if (content.step > 0.0f)
                value = content.minimum + std::round((value - content.minimum) / content.step) * content.step;
            value = std::min(content.maximum, std::max(content.minimum, value));
            if (value == content.value)
                return false;
            content.value = value;
            ui::markDirty(slider, ui::DIRTY_PAINT);
            if (!content.onChange.empty())
                state.actions->action(content.onChange, slider);
            return true;
        }
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

        setValue(state, slider, slider.content.minimum + t * (slider.content.maximum - slider.content.minimum));
    }

    // Стрелки: шаг или сотая доля диапазона / The arrows: the step or a hundredth of the range
    bool stepSlider(RouterState& state, ui::Node& slider, int direction)
    {
        float step = slider.content.step;

        if (step <= 0.0f)
            step = (slider.content.maximum - slider.content.minimum) / 100.0f;
        return setValue(state, slider, slider.content.value + step * static_cast<float>(direction));
    }
}
