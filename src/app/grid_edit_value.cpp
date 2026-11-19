#include "gran/app/internal/app_state.hpp"
#include "gran/data/cell_value.hpp"

#include <algorithm>
#include <locale>
#include <sstream>

namespace gran::app
{
    namespace
    {
        // Число ползунка текстом с точкой при любой локали (как setVariableNumber) / The slider number as text with a dot under any locale (as setVariableNumber)
        std::string numberText(float value)
        {
            std::ostringstream text;

            text.imbue(std::locale::classic());
            text.precision(7);
            text << value;
            return text.str();
        }

        // Текст → число ползунка в его пределах; не число — значение не меняется / A text → the slider number within its limits; not a number — the value does not change
        void setSliderText(ui::Node& slider, const std::string& text)
        {
            std::string number = text;
            float value = 0.0f;

            if (!data::normalizeValue(data::ColumnType::NUMBER, number) || number.empty())
                return;
            std::istringstream input(number);
            input.imbue(std::locale::classic());
            input >> value;
            if (input.fail())
                return;
            slider.content.value = std::max(slider.content.minimum, std::min(value, slider.content.maximum));
        }
    }

    std::string readEditorValue(const ui::Node& value)
    {
        ui::ElementKind kind = value.kind;

        if (kind == ui::ElementKind::CHECKBOX || kind == ui::ElementKind::SWITCH)
        {
            if (value.content.checked)
                return "true";
            return "false";
        }
        if (kind == ui::ElementKind::SLIDER)
            return numberText(value.content.value);
        if (kind == ui::ElementKind::COMBO_BOX)
            return value.content.option;
        return value.content.text;
    }

    /*
    ** Список выбирает пункт с этим значением (подпись — его текст); нет такого пункта — выбор снимается.
    ** Поле ввода: выделение от начала до конца (select) или каретка в конце набранного символа.
    **
    ** A combo box picks the item with this value (the label is its text); no such item — the choice is dropped.
    ** An input: the selection from the start to the end (select) or the caret at the end of the typed character.
    */
    void writeEditorValue(ui::Node& value, const std::string& text, bool select)
    {
        ui::ElementKind kind = value.kind;

        if (kind == ui::ElementKind::CHECKBOX || kind == ui::ElementKind::SWITCH)
            value.content.checked = data::valueTrue(text);
        else if (kind == ui::ElementKind::SLIDER)
            setSliderText(value, text);
        else if (kind == ui::ElementKind::COMBO_BOX)
            ui::selectOption(value, ui::findOption(value, text));
        else
        {
            value.content.text = text;
            value.content.caret = text.size();
            value.content.anchor = text.size();
            if (select)
                value.content.anchor = 0;
        }
        ui::markDirty(value, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }
}
