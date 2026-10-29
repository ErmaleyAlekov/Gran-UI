#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Поля мыши переносятся из события окна как есть (координаты уже логические) / The mouse fields are carried over from the window event as they are (the coordinates are already logical)
    ui::UiEvent pointerEvent(ui::EventType type, ui::Node& target, const platform::Event& source)
    {
        ui::UiEvent event = ui::makeEvent(type, target);

        event.position = source.position;
        event.wheel = source.wheel;
        event.button = source.button;
        event.modifiers = source.modifiers;
        event.clicks = source.clicks;
        return event;
    }

    // Захват важнее попадания: тянущий ползунок своего вида не теряет мышь за краем / The capture beats the hit: a custom slider being dragged does not lose the mouse past its edge
    ui::Node* pointerTarget(const RouterState& state, ui::Node* hit) noexcept
    {
        if (state.capture != nullptr)
            return state.capture;
        return hit;
    }

    // Мышь над пустым местом (или под модальным слоем) — событию некуда идти, действие разрешено
    // The mouse over an empty place (or under a modal layer) — the event has nowhere to go, the action is allowed
    bool firePointer(RouterState& state, ui::EventType type, ui::Node* target, const platform::Event& source)
    {
        if (target == nullptr)
            return true;
        ui::UiEvent event = pointerEvent(type, *target, source);
        return fire(state, event);
    }
}
