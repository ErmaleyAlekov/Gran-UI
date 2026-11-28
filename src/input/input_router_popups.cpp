#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Открытие из кода: то же, что открытие меню вводом / Opening from code: the same as opening a menu by input
    void InputRouter::openPopup(ui::Node& popup, const Rect& anchor, bool passive)
    {
        openLayer(_state, popup, anchor, passive);
        flushEvents(_state);
    }

    // Переносим стек живого диалога без событий открытия и промежуточной смены фокуса.
    // Transfer the live dialog stack without opening events or intermediate focus changes.
    void InputRouter::adoptProfileInput(InputRouter& previous, ui::Node* dialog, ui::Node* focus)
    {
        auto& layers = previous._state.layers;
        std::size_t count = 0;
        for (auto entry : layers)
        {
            if (!dialog || !within(entry.node, *dialog)) continue;
            auto* opener = entry.returnFocus;
            if (opener && !within(opener, *_state.root))
                entry.returnFocus = opener->id.empty() ? nullptr : ui::findNode(*_state.root, opener->id);
            layers[count++] = entry;
        }
        layers.resize(count);
        _state.layers = std::move(layers);
        auto* old = previous._state.focus;
        if (focus && old && (focus == old || (!old->id.empty() && old->id == focus->id && old->kind == focus->kind)))
        {
            _state.focus = focus;
            setNodeState(*focus, style::stateBit(style::State::FOCUSED), true);
            if (ui::isTextField(*focus)) _state.selection = focus;
        }
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
