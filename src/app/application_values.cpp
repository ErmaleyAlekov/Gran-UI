#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <algorithm>

namespace gran
{
    namespace
    {
        // Вид не из списка подходящих — ошибка программиста (список видов — в тексте ошибки)
        // A kind not from the fitting list is a programmer error (the kind list is in the error text)
        void requireKinds(const ui::Node& node, ui::ElementKind first, ui::ElementKind second, const char* expected)
        {
            if (node.kind != first && node.kind != second)
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_WRONG_ELEMENT,
                    {ui::elementName(node.kind), expected}));
        }
    }

    // Значение не выходит за min и max (как проверила бы схема у value в разметке) / The value does not leave min and max (as the schema would check value in the markup)
    void Application::setValue(ui::Node& node, float value)
    {
        requireKinds(node, ui::ElementKind::PROGRESS_BAR, ui::ElementKind::SLIDER, "ProgressBar, Slider");
        value = std::min(node.content.maximum, std::max(node.content.minimum, value));
        if (node.content.value == value)
            return;
        node.content.value = value;
        ui::markDirty(node, ui::DIRTY_PAINT);
    }

    /*
    ** Радиокнопка отмечается через группу (остальные снимаются); снять отметку можно и у неё —
    ** тогда в группе не отмечено ничего, как у радиокнопок HTML до первого выбора.
    **
    ** A radio button is checked through its group (the others are unchecked); its mark can be cleared too —
    ** then nothing is checked in the group, as for HTML radio buttons before the first choice.
    */
    void Application::setChecked(ui::Node& node, bool checked)
    {
        if (node.kind != ui::ElementKind::RADIO_BUTTON)
            requireKinds(node, ui::ElementKind::CHECKBOX, ui::ElementKind::SWITCH, "CheckBox, Switch, RadioButton");
        if (node.kind == ui::ElementKind::RADIO_BUTTON && checked)
            ui::selectInGroup(node);
        else
            ui::setChecked(node, checked);
    }
}
