#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Движение: новый узел под мышью; при перетаскивании — значение ползунка / A move: a new node under the mouse; while dragging — the slider value
        void pointerMove(RouterState& state, const platform::Event& event)
        {
            state.pointer = event.position;
            changeHover(state, spanAt(state, pickNode(state, event.position), event.position));
            hoverMenus(state, state.hover);
            dragTextSelection(state, event.position);
            if (state.drag != nullptr)
                dragSlider(state, *state.drag, ui::windowToNode(*state.drag, event.position).x);
        }

        /*
        ** Нажатие мимо открытого меню только закрывает его; правая кнопка открывает контекстное меню.
        ** Нажатие левой кнопкой: элемент управления становится нажатым и получает фокус
        ** (нажатие мимо элементов снимает фокус), ползунок начинает перетаскивание,
        ** в поле ввода и Text начинается выделение (нажатие мимо текста его снимает).
        **
        ** A press outside an open menu only closes it; the right button opens the context menu.
        ** A left button press: the control becomes pressed and gets the focus
        ** (a press outside the controls removes the focus), a slider starts dragging,
        ** in an input and a Text a selection starts (a press outside the text removes it).
        */
        void pointerDown(RouterState& state, const platform::Event& event)
        {
            ui::Node* hit = spanAt(state, pickNode(state, event.position), event.position);
            ui::Node* control = controlOf(hit);

            if (closeOutside(state, event.position))
                return;
            if (event.button == platform::MouseButton::RIGHT)
                contextClick(state, event);
            if (event.button != platform::MouseButton::LEFT)
                return;
            if (control != nullptr && disabled(control))
                control = nullptr;
            ui::Node* text = textTarget(hit, control);
            // Нажатие в меню выделение не снимает: команда «Копировать» берёт именно его
            // A press in a menu does not drop the selection: the "Copy" command takes exactly it
            if (text != nullptr)
                beginTextSelection(state, *text, event);
            else if (ui::popupOf(hit) == nullptr)
                clearSelection(state);
            changePressed(state, control);
            changeFocus(state, control);
            if (control != nullptr && control->kind == ui::ElementKind::SLIDER)
            {
                state.drag = control;
                dragSlider(state, *control, ui::windowToNode(*control, event.position).x);
            }
        }

        // Отпускание над тем же элементом — щелчок (кроме ползунка: его значение уже менялось)
        // A release over the same control is a click (except a slider: its value was already changing)
        void pointerUp(RouterState& state, const platform::Event& event)
        {
            ui::Node* control = controlOf(spanAt(state, pickNode(state, event.position), event.position));
            ui::Node* pressed = state.pressed;

            if (event.button != platform::MouseButton::LEFT)
                return;
            changePressed(state, nullptr);
            state.drag = nullptr;
            state.selecting = nullptr;
            if (pressed != nullptr && pressed == control && pressed->kind != ui::ElementKind::SLIDER)
                activate(state, *pressed);
        }

        // Колесо: ближайшая прокрутка сдвигается на 48 логических пикселей за «щелчок»; под диалогом прокрутки нет
        // The wheel: the nearest scroll moves by 48 logical pixels per "notch"; there is no scrolling under a dialog
        void pointerWheel(RouterState& state, const platform::Event& event)
        {
            ui::Node* scroll = scrollOf(pickNode(state, event.position));

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
