#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Своё событие из кода могло сменить фокус обработчиком — уведомления доставляются сразу / A custom event from code might change the focus by a handler — the notifications are delivered at once
    bool InputRouter::dispatch(ui::UiEvent& event)
    {
        bool allowed = dispatchEvent(event, *_state.actions);

        flushEvents(_state);
        return allowed;
    }

    // Захват из обработчика pointerDown действует до отпускания кнопки / A capture from a pointerDown handler holds until the button release
    void InputRouter::capturePointer(ui::Node* node) noexcept
    {
        _state.capture = node;
    }

    // Узел захвата / The capture node
    const ui::Node* InputRouter::pointerCapture() const noexcept
    {
        return _state.capture;
    }

    // Кандидат до порога ещё не перетаскивание / A candidate before the threshold is not a drag yet
    bool InputRouter::draggingNode() const noexcept
    {
        return _state.nodeDrag.active;
    }
}
