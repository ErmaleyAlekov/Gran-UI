#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Порог начала в логических пикселях: дрожание руки при щелчке не начинает перетаскивание
        // The start threshold in logical pixels: a hand tremor on a click does not start a drag
        constexpr float DRAG_THRESHOLD = 4.0f;

        /*
        ** Мышь ушла дальше порога — dragStart источнику (точка — место нажатия). Отмена или удаление
        ** источника — кандидат забывается. Начавшееся перетаскивание снимает нажатие и выделение мышью.
        **
        ** The mouse went past the threshold — dragStart to the source (the point is the press place). Cancelling or removing
        ** the source — the candidate is forgotten. A started drag removes the press and the mouse selection.
        */
        bool startDrag(RouterState& state, const platform::Event& event)
        {
            NodeDrag& drag = state.nodeDrag;
            float dx = event.position.x - drag.origin.x;
            float dy = event.position.y - drag.origin.y;

            if (dx * dx + dy * dy < DRAG_THRESHOLD * DRAG_THRESHOLD)
                return false;
            ui::UiEvent start = dragEvent(state, ui::EventType::DRAG_START, *drag.source, event);
            start.position = drag.origin;
            if (!fire(state, start) || drag.source == nullptr)
            {
                drag = NodeDrag{};
                return false;
            }
            drag.data = start.drag;
            drag.active = true;
            changePressed(state, nullptr);
            state.selecting = nullptr;
            state.capture = nullptr;
            setNodeState(*drag.source, style::stateBit(style::State::DRAGGING), true);
            return true;
        }

        // Конец: состояния сняты, dragEnd источнику (если он ещё в дереве) — последним, когда поля уже сброшены
        // The end: the states are removed, dragEnd to the source (if it is still in the tree) — last, when the fields are already reset
        void endDrag(RouterState& state, const platform::Event& source, bool dropped)
        {
            ui::Node* node = state.nodeDrag.source;
            ui::UiEvent end;

            if (node != nullptr)
            {
                setNodeState(*node, style::stateBit(style::State::DRAGGING), false);
                end = dragEvent(state, ui::EventType::DRAG_END, *node, source);
                end.dropped = dropped;
            }
            changeZone(state, nullptr, source, false);
            state.nodeDrag = NodeDrag{};
            if (node != nullptr)
                fire(state, end);
        }
    }

    /*
    ** Пока идёт перетаскивание, события мыши поглощаются: зона под мышью получает dragEnter/dragLeave
    ** и dragOver; отменённый dragOver — «здесь нельзя» (нет DropTarget, курсор «нельзя», drop не будет).
    **
    ** While a drag goes on, the mouse events are consumed: the zone under the mouse gets dragEnter/dragLeave
    ** and dragOver; a cancelled dragOver means "not here" (no DropTarget, the "no" cursor, there will be no drop).
    */
    bool moveDrag(RouterState& state, const platform::Event& event)
    {
        NodeDrag& drag = state.nodeDrag;

        if (drag.source == nullptr || (!drag.active && !startDrag(state, event)))
            return false;
        changeZone(state, dropZoneAt(state, event.position), event, true);
        if (drag.source == nullptr)
        {
            cancelDrag(state);
            return true;
        }
        drag.allowed = false;
        if (drag.zone != nullptr)
        {
            ui::UiEvent over = dragEvent(state, ui::EventType::DRAG_OVER, *drag.zone, event);
            drag.allowed = fire(state, over) && drag.zone != nullptr;
        }
        if (drag.zone != nullptr)
            setNodeState(*drag.zone, style::stateBit(style::State::DROP_TARGET), drag.allowed);
        return true;
    }

    /*
    ** Сброс в согласную зону — drop (всплывает к предкам зоны); отменённый drop — зона отказала.
    ** Обработчик drop обычно переносит запись модели — это удаляет узлы, но сброс уже состоялся.
    **
    ** A drop into an agreeing zone is drop (it bubbles to the zone ancestors); a cancelled drop means the zone refused.
    ** A drop handler usually moves a model record — this removes nodes, but the drop has already happened.
    */
    bool finishDrag(RouterState& state, const platform::Event& event)
    {
        NodeDrag& drag = state.nodeDrag;
        bool dropped = false;

        if (!drag.active)
            return false;
        if (drag.zone != nullptr && drag.allowed && drag.source != nullptr)
        {
            ui::UiEvent drop = dragEvent(state, ui::EventType::DROP, *drag.zone, event);
            fire(state, drop);
            dropped = !drop.defaultPrevented;
        }
        endDrag(state, event, dropped);
        return true;
    }

    // Кандидат без начавшегося перетаскивания просто забывается / A candidate without a started drag is just forgotten
    void cancelDrag(RouterState& state)
    {
        platform::Event source;

        source.position = state.pointer;
        if (!state.nodeDrag.active)
        {
            state.nodeDrag = NodeDrag{};
            return;
        }
        changeZone(state, nullptr, source, true);
        endDrag(state, source, false);
    }
}
