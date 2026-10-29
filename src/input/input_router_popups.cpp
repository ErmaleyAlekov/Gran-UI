#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Открытие из кода: то же, что открытие меню вводом / Opening from code: the same as opening a menu by input
    void InputRouter::openPopup(ui::Node& popup, const Rect& anchor, bool passive)
    {
        openLayer(_state, popup, anchor, passive);
        flushEvents(_state);
    }

    // Закрытого слоя в списке нет — закрытие ничего не меняет, кроме видимости / A closed layer is not in the list — closing changes nothing but the visibility
    void InputRouter::closePopup(ui::Node& popup)
    {
        closeLayer(_state, popup);
        flushEvents(_state);
    }

    // Для подсказок: над каким узлом стоит мышь / For tooltips: which node the mouse stands over
    const ui::Node* InputRouter::hovered() const noexcept
    {
        return _state.hover;
    }

    // Точка последнего движения мыши (место подсказки) / The point of the last mouse move (the tooltip place)
    Vec2 InputRouter::pointer() const noexcept
    {
        return _state.pointer;
    }
}
