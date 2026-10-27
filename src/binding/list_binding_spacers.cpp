#include "gran/binding/internal/list_binding.hpp"

#include <cmath>

namespace gran::binding
{
    namespace
    {
        /*
        ** Размер распорки по главной оси — объявление width/height узла (как атрибут разметки).
        ** Меньше полупикселя разницы — не изменение: иначе дрожание среднего размера
        ** перекомпоновывало бы список каждый кадр. true — распорка изменилась.
        **
        ** The spacer size along the main axis is the node width/height declaration (like a markup attribute).
        ** Less than half a pixel of difference is not a change: otherwise the average size jitter
        ** would relayout the list every frame. true — the spacer changed.
        */
        bool setMainSize(ui::Node& spacer, float size, bool horizontal)
        {
            style::PropertyId property = style::PropertyId::HEIGHT;
            bool visible = size > 0.0f;

            if (horizontal)
                property = style::PropertyId::WIDTH;
            const style::PropertyValue* current = style::findDeclaration(spacer.declared.base, property);
            const style::Length* length = nullptr;
            if (current != nullptr)
                length = std::get_if<style::Length>(current);
            bool same = length != nullptr && std::fabs(length->value - size) < 0.5f;
            if (same && spacer.visible == visible)
                return false;
            style::setDeclaration(spacer.declared.base, property, style::Length{std::fmax(size, 0.0f),
                style::LengthUnit::PIXELS});
            spacer.visible = visible;
            ui::markDirty(spacer, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            return true;
        }
    }

    /*
    ** Распорка занимает место своих записей без одного промежутка: его даёт сам стек между
    ** распоркой и первой копией. Пока размер записи не измерен (до первой компоновки), распорки нулевые.
    **
    ** A spacer takes the place of its records minus one gap: the stack itself gives that gap between
    ** the spacer and the first copy. While the record size is not measured (before the first layout), the spacers are zero.
    */
    bool ListBinding::applySpacers()
    {
        bool horizontal = _container.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
        float gap = _container.computed.spacing;
        std::size_t after = _model.size() - (_first + _items.size());
        float leadSize = 0.0f;
        float trailSize = 0.0f;

        if (!_virtual)
            return false;
        if (_first > 0 && _extent > 0.0f)
            leadSize = static_cast<float>(_first) * _extent - gap;
        if (after > 0 && _extent > 0.0f)
            trailSize = static_cast<float>(after) * _extent - gap;
        bool changed = setMainSize(*_lead, leadSize, horizontal);
        return setMainSize(*_trail, trailSize, horizontal) || changed;
    }
}
