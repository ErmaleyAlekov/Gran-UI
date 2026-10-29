#include "gran/ui/ui_event.hpp"

#include <cstring>

namespace gran::ui
{
    namespace
    {
        // Имена в порядке EventType (без CUSTOM) / The names in the EventType order (without CUSTOM)
        const char* const NAMES[] = {"keyDown", "keyUp", "pointerDown", "pointerMove", "pointerUp", "pointerEnter",
            "pointerLeave", "doubleClick", "wheel", "focus", "blur", "contextMenu", "dragStart", "dragEnter", "dragOver",
            "dragLeave", "drop", "dragEnd"};
    }

    // Флаг пути; обработчики узла current довызываются / The path flag; the handlers of the current node are still called
    void UiEvent::stopPropagation() noexcept
    {
        propagationStopped = true;
    }

    // Оба флага сразу / Both flags at once
    void UiEvent::stopImmediatePropagation() noexcept
    {
        propagationStopped = true;
        immediateStopped = true;
    }

    // Неотменяемое событие (focus, pointerMove…) флаг не ставит — как в DOM / A non-cancelable event (focus, pointerMove…) does not set the flag — as in the DOM
    void UiEvent::preventDefault() noexcept
    {
        if (cancelable)
            defaultPrevented = true;
    }

    // Своё событие имени вида не имеет / A custom event has no kind name
    const char* eventName(EventType type) noexcept
    {
        std::size_t index = static_cast<std::size_t>(type);

        if (index >= sizeof(NAMES) / sizeof(NAMES[0]))
            return "";
        return NAMES[index];
    }

    // Поиск по таблице: событий мало, хэш не нужен / A table search: there are few events, a hash is not needed
    EventType eventType(const std::string& name) noexcept
    {
        for (std::size_t index = 0; index < sizeof(NAMES) / sizeof(NAMES[0]); ++index)
        {
            if (std::strcmp(NAMES[index], name.c_str()) == 0)
                return static_cast<EventType>(index);
        }
        return EventType::CUSTOM;
    }
}
