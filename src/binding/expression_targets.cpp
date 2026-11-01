#include "gran/binding/internal/expression_binding.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/property.hpp"

#include "beresta.hpp"

#include <algorithm>
#include <cmath>

namespace gran::binding
{
    namespace
    {
        // Ложь — "false", "0" и пусто; остальное истинно (как непустая строка в XPath) / False is "false", "0" and empty; the rest is true (as a non-empty string in XPath)
        bool parseFlag(const std::string& value) noexcept
        {
            return !(value.empty() || value == "false" || value == "0");
        }

        // visible, enabled, checked; false — значение то же или атрибут не логический / visible, enabled, checked; false — the same value or the attribute is not boolean
        bool applyFlag(ui::Node& node, const std::string& attribute, bool flag)
        {
            if (attribute == "visible" && node.visible != flag)
            {
                node.visible = flag;
                ui::markDirty(node, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
                return true;
            }
            if (attribute == "enabled" && node.enabled != flag)
            {
                ui::setEnabled(node, flag);
                return true;
            }
            if (attribute != "checked" || node.content.checked == flag)
                return false;
            // Отметка радиокнопки снимает её с остальных в группе / Checking a radio button unchecks the others in the group
            if (node.kind == ui::ElementKind::RADIO_BUTTON && flag)
                ui::selectInGroup(node);
            else
                ui::setChecked(node, flag);
            return true;
        }

        /*
        ** value: число ползунка и индикатора (в пределах min и max, не число — без изменений), выбор
        ** ComboBox и Tabs, значение пункта у прочих (MenuItem, RadioButton, Tab).
        **
        ** value: the number of a slider and a progress bar (within min and max, not a number — no change), the choice
        ** of a ComboBox and Tabs, the item value for the others (MenuItem, RadioButton, Tab).
        */
        bool applyValue(ui::Node& node, const std::string& value)
        {
            if (node.kind == ui::ElementKind::SLIDER || node.kind == ui::ElementKind::PROGRESS_BAR)
            {
                double number = beresta::xpath::stringToNumber(value);
                if (std::isnan(number))
                    return false;
                float clamped = std::min(node.content.maximum, std::max(node.content.minimum, static_cast<float>(number)));
                if (node.content.value == clamped)
                    return false;
                node.content.value = clamped;
                ui::markDirty(node, ui::DIRTY_PAINT);
                return true;
            }
            if (node.kind == ui::ElementKind::COMBO_BOX)
                return ui::selectOption(node, ui::findOption(node, value));
            if (node.kind == ui::ElementKind::TABS)
                return ui::selectTab(node, value);
            if (node.content.option == value)
                return false;
            node.content.option = value;
            ui::markDirty(node, ui::DIRTY_PAINT);
            return true;
        }

        // Свойство стиля (width, opacity, background…); значение, которое не разбирается, не применяется / A style property (width, opacity, background…); a value that does not parse is not applied
        bool applyStyle(ui::Node& node, const std::string& attribute, const std::string& value)
        {
            const style::PropertyInfo* info = style::findProperty(attribute);

            if (info == nullptr)
                return false;
            try
            {
                style::setDeclaration(node.declared.base, info->id, style::parsePropertyValue(*info, value));
            }
            catch (const GranException&)
            {
                return false;
            }
            ui::markDirty(node, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            return true;
        }
    }

    // Текст из выражения — значение из кода: история правок поля сбрасывается, каретка — в конец (как writeProperty) / A text from an expression is a value from code: the field edit history is reset, the caret goes to the end (as writeProperty)
    bool applyTarget(ui::Node& node, const std::string& attribute, const std::string& value)
    {
        std::string ui::NodeContent::* member = textMember(attribute);

        if (booleanTarget(attribute))
            return applyFlag(node, attribute, parseFlag(value));
        if (attribute == "value")
            return applyValue(node, value);
        if (member == nullptr)
            return applyStyle(node, attribute, value);
        std::string& slot = node.content.*member;
        if (slot == value)
            return false;
        slot = value;
        if (member == &ui::NodeContent::text)
        {
            node.content.history.reset();
            node.content.caret = value.size();
            node.content.anchor = value.size();
        }
        ui::markDirty(node, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        if (node.kind == ui::ElementKind::SPAN && node.parent != nullptr)
            ui::joinSpans(*node.parent);
        return true;
    }
}
