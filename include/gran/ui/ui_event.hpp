#ifndef GRAN_UI_UI_EVENT_HPP
#define GRAN_UI_UI_EVENT_HPP

#include "gran/platform/window.hpp"
#include "gran/utils/geometry.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace gran::ui
{
    struct Node;

    /*
    ** Вид события интерфейса. Имена для разметки и кода — eventName (pointerDown, keyDown…); атрибут разметки —
    ** on + имя с заглавной (onPointerDown), с суффиксом Capture — фаза захвата. CUSTOM — своё событие приложения
    ** с любым именем (Application::dispatchEvent, атрибут on="имя:действие").
    **
    ** The UI event kind. The names for markup and code are eventName (pointerDown, keyDown…); the markup attribute is
    ** on + the capitalized name (onPointerDown), with the Capture suffix — the capture phase. CUSTOM is an application event
    ** with any name (Application::dispatchEvent, the on="name:action" attribute).
    */
    enum class EventType : std::uint8_t
    {
        KEY_DOWN, KEY_UP, POINTER_DOWN, POINTER_MOVE, POINTER_UP, POINTER_ENTER, POINTER_LEAVE, DOUBLE_CLICK, WHEEL,
        FOCUS, BLUR, CONTEXT_MENU, DRAG_START, DRAG_ENTER, DRAG_OVER, DRAG_LEAVE, DROP, DRAG_END, CUSTOM
    };

    /*
    ** Фаза доставки, как в DOM: захват (от корня к цели), цель, всплытие (от цели к корню). Номера — как
    ** Event.eventPhase в браузерах (их же видит Си-интерфейс).
    **
    ** The delivery phase, as in the DOM: capture (from the root to the target), the target, bubbling (from the target to the root).
    ** The numbers are as Event.eventPhase in browsers (the C interface sees the same ones).
    */
    enum class EventPhase : std::uint8_t { NONE = 0, CAPTURE = 1, TARGET = 2, BUBBLE = 3 };

    /*
    ** Данные внутреннего перетаскивания узла: формат (по нему зона решает, принять ли: acceptDrop) и строка
    ** данных. По умолчанию формат — dragFormat узла (без него — "node"), данные — dragData (без него — id).
    **
    ** The internal node drag data: the format (a zone decides by it whether to accept: acceptDrop) and the data
    ** string. By default the format is the node dragFormat (without it — "node"), the data is dragData (without it — the id).
    */
    struct DragData
    {
        std::string format;
        std::string data;
    };

    /*
    ** Событие интерфейса — один объект на всю доставку: обработчики видят и меняют его флаги
    ** (stopPropagation, preventDefault), как в DOM.
    **
    ** target — узел, которому событие адресовано; current — узел, чей обработчик вызван сейчас; related — второй
    ** участник: узел, откуда/куда ушёл указатель или фокус (enter/leave, focus/blur), источник перетаскивания
    ** (у событий зоны) или зона (у dragEnd). position — точка окна в логических пикселях, local — та же точка
    ** в системе current (с его преобразованиями, от левого верхнего угла рамки узла).
    **
    ** A UI event is one object for the whole delivery: the handlers see and change its flags
    ** (stopPropagation, preventDefault), as in the DOM.
    **
    ** target is the node the event is addressed to; current is the node whose handler is called now; related is the second
    ** participant: the node the pointer or the focus came from/went to (enter/leave, focus/blur), the drag source
    ** (for the zone events) or the zone (for dragEnd). position is the window point in logical pixels, local is the same point
    ** in the current system (with its transforms, from the top left corner of the node frame).
    */
    struct UiEvent
    {
        // ---- ui_event.cpp ----

        // Не передавать событие дальше по пути (обработчики текущего узла ещё вызываются)
        // Do not pass the event further along the path (the handlers of the current node are still called)
        void stopPropagation() noexcept;

        // Остановить сразу: и путь, и оставшиеся обработчики текущего узла / Stop at once: both the path and the remaining handlers of the current node
        void stopImmediatePropagation() noexcept;

        // Отменить действие движка по умолчанию (только у cancelable) / Cancel the engine default action (only for cancelable)
        void preventDefault() noexcept;

        EventType type = EventType::CUSTOM;
        // Имя вида: eventName(type) или имя своего события / The kind name: eventName(type) or the custom event name
        std::string name;
        Node* target = nullptr;
        Node* current = nullptr;
        Node* related = nullptr;
        EventPhase phase = EventPhase::NONE;
        Vec2 position;
        Vec2 local;
        // Сдвиг колеса в «щелчках» (wheel) / The wheel shift in "notches" (wheel)
        Vec2 wheel;
        platform::MouseButton button = platform::MouseButton::LEFT;
        platform::Key key = platform::Key::UNKNOWN;
        std::uint8_t modifiers = 0;
        // Число щелчков подряд (pointerDown: 2 — второе нажатие двойного щелчка) / The click count in a row (pointerDown: 2 — the second press of a double click)
        std::uint8_t clicks = 1;
        // Строка своего события (dispatchEvent) / The custom event string (dispatchEvent)
        std::string detail;
        DragData drag;
        // dragEnd: перетаскивание закончилось сбросом в зону (false — отмена или зона отказала)
        // dragEnd: the drag ended with a drop into a zone (false — cancelled or the zone refused)
        bool dropped = false;
        bool bubbles = true;
        bool cancelable = true;
        bool propagationStopped = false;
        bool immediateStopped = false;
        bool defaultPrevented = false;
        // Сколько обработчиков вызвано (0 — событие никто не слушал) / How many handlers were called (0 — nobody listened)
        std::uint32_t handled = 0;
    };

    // Обработчик события: узел, чей обработчик вызван (current), и событие / An event handler: the node whose handler is called (current) and the event
    using EventHandler = std::function<void(Node& node, UiEvent& event)>;

    // Номер слушателя из кода (0 — «нет слушателя») / A listener number from code (0 — "no listener")
    using ListenerId = std::uint64_t;

    // Обработчик из разметки: onPointerDown="действие" и т. п. / A handler from the markup: onPointerDown="action" and so on
    struct EventBinding
    {
        EventType type = EventType::CUSTOM;
        // Имя своего события (только CUSTOM) / The custom event name (CUSTOM only)
        std::string name;
        bool capture = false;
        std::string action;
    };

    // Слушатель из кода (Application::addEventListener); живёт в узле и уходит вместе с ним
    // A listener from code (Application::addEventListener); it lives in the node and goes away together with it
    struct EventListener
    {
        EventType type = EventType::CUSTOM;
        std::string name;
        bool capture = false;
        ListenerId id = 0;
        EventHandler handler;
    };

    // ---- ui_event.cpp ----

    // Имя вида события ("pointerDown"); CUSTOM — пустая строка / The event kind name ("pointerDown"); CUSTOM — an empty string
    const char* eventName(EventType type) noexcept;

    // Вид по имени; неизвестное имя — CUSTOM (своё событие) / The kind by the name; an unknown name — CUSTOM (a custom event)
    EventType eventType(const std::string& name) noexcept;

    // ---- ui_event_make.cpp ----

    /*
    ** Событие вида type для узла target с флагами как в DOM: pointerEnter/Leave, focus/blur, dragEnter/Leave
    ** не всплывают (их видят захват и сама цель); их же и dragEnd отменить нельзя. Своё событие — name,
    ** всплывает и отменяется.
    **
    ** An event of the type kind for the target node with the flags as in the DOM: pointerEnter/Leave, focus/blur, dragEnter/Leave
    ** do not bubble (the capture and the target itself see them); they and dragEnd cannot be cancelled. A custom event is name,
    ** it bubbles and can be cancelled.
    */
    UiEvent makeEvent(EventType type, Node& target, const std::string& name = std::string());
}

#endif
