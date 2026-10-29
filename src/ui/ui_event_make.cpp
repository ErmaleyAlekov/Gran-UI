#include "gran/ui/ui_event.hpp"

namespace gran::ui
{
    namespace
    {
        // Уведомление о смене (указатель, фокус, зона): только захват и цель / A change notification (the pointer, the focus, a zone): the capture and the target only
        bool notification(EventType type) noexcept
        {
            return type == EventType::POINTER_ENTER || type == EventType::POINTER_LEAVE || type == EventType::FOCUS
                || type == EventType::BLUR || type == EventType::DRAG_ENTER || type == EventType::DRAG_LEAVE;
        }
    }

    // Имя своего события задаёт вызывающий; встроенное — из таблицы / The custom event name is given by the caller; a built-in one — from the table
    UiEvent makeEvent(EventType type, Node& target, const std::string& name)
    {
        UiEvent event;

        event.type = type;
        event.name = name;
        if (type != EventType::CUSTOM)
            event.name = eventName(type);
        event.target = &target;
        event.bubbles = !notification(type);
        event.cancelable = !notification(type) && type != EventType::DRAG_END;
        return event;
    }
}
