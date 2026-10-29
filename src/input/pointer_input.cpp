#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        /*
        ** Движение: новый узел под мышью (pointerLeave/pointerEnter — до pointerMove, как в браузере),
        ** перетаскивание узла; иначе pointerMove цели (узлу захвата или под мышью), затем действия
        ** по умолчанию — подменю, выделение мышью, значение ползунка.
        **
        ** A move: a new node under the mouse (pointerLeave/pointerEnter — before pointerMove, as in a browser),
        ** a node drag; otherwise pointerMove to the target (the capture node or the one under the mouse), then the default
        ** actions — the submenus, the mouse selection, the slider value.
        */
        void pointerMove(RouterState& state, const platform::Event& event)
        {
            state.pointer = event.position;
            changeHover(state, spanAt(state, pickNode(state, event.position), event.position));
            flushEvents(state);
            if (moveDrag(state, event))
                return;
            if (!firePointer(state, ui::EventType::POINTER_MOVE, pointerTarget(state, state.hover), event))
                return;
            hoverMenus(state, state.hover);
            dragTextSelection(state, event.position);
            if (state.drag != nullptr)
                dragSlider(state, *state.drag, ui::windowToNode(*state.drag, event.position).x);
        }

        /*
        ** Нажатие мимо открытого меню только закрывает его (событий нет). Иначе pointerDown цели; если он
        ** не отменён — правая кнопка: contextMenu и встроенное меню, левая — нажатие элемента (pressLeft).
        **
        ** A press outside an open menu only closes it (no events). Otherwise pointerDown to the target; if it
        ** is not cancelled — the right button: contextMenu and the built-in menu, the left one — pressing a control (pressLeft).
        */
        void pointerDown(RouterState& state, const platform::Event& event)
        {
            ui::Node* hit = spanAt(state, pickNode(state, event.position), event.position);

            if (closeOutside(state, event.position))
                return;
            if (event.button == platform::MouseButton::LEFT)
                state.downClicks = event.clicks;
            if (!firePointer(state, ui::EventType::POINTER_DOWN, pointerTarget(state, hit), event))
                return;
            if (event.button == platform::MouseButton::RIGHT
                && firePointer(state, ui::EventType::CONTEXT_MENU, hit, event))
                contextClick(state, event);
            if (event.button == platform::MouseButton::LEFT)
                pressLeft(state, hit, event);
        }

        // Отпускание: конец перетаскивания узла или pointerUp; захват снимается; левая кнопка — щелчок (releaseLeft)
        // A release: the end of a node drag or pointerUp; the capture is removed; the left button — a click (releaseLeft)
        void pointerUp(RouterState& state, const platform::Event& event)
        {
            ui::Node* hit = spanAt(state, pickNode(state, event.position), event.position);
            bool allowed = false;

            if (event.button != platform::MouseButton::LEFT || !finishDrag(state, event))
                allowed = firePointer(state, ui::EventType::POINTER_UP, pointerTarget(state, hit), event);
            state.capture = nullptr;
            if (event.button == platform::MouseButton::LEFT)
                releaseLeft(state, hit, event, allowed);
        }

        // Колесо: wheel узлу под мышью, затем ближайшая прокрутка — на 48 логических пикселей за «щелчок»; под диалогом прокрутки нет
        // The wheel: wheel to the node under the mouse, then the nearest scroll — by 48 logical pixels per "notch"; there is no scrolling under a dialog
        void pointerWheel(RouterState& state, const platform::Event& event)
        {
            ui::Node* hit = pickNode(state, event.position);

            if (!firePointer(state, ui::EventType::WHEEL, hit, event))
                return;
            ui::Node* scroll = scrollOf(hit);
            if (scroll == nullptr)
                return;
            scroll->layout.scrollOffset.x -= event.wheel.x * 48.0f;
            scroll->layout.scrollOffset.y -= event.wheel.y * 48.0f;
            ui::markDirty(*scroll, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        }
    }

    // Распределение событий мыши / Dispatching the mouse events
    bool handlePointer(RouterState& state, const platform::Event& event)
    {
        switch (event.kind)
        {
            case platform::EventKind::MOUSE_MOVE: pointerMove(state, event); return true;
            case platform::EventKind::MOUSE_LEAVE: changeHover(state, nullptr); return true;
            case platform::EventKind::MOUSE_DOWN: pointerDown(state, event); return true;
            case platform::EventKind::MOUSE_UP: pointerUp(state, event); return true;
            case platform::EventKind::MOUSE_WHEEL: pointerWheel(state, event); return true;
            default: return false;
        }
    }
}
