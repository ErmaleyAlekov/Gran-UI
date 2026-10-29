#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Цель клавиатуры: фокус; без него — верхний слой (диалог, меню), иначе корень (как body в браузере)
        // The keyboard target: the focus; without it — the top layer (a dialog, a menu), otherwise the root (as body in a browser)
        ui::Node& keyTarget(RouterState& state) noexcept
        {
            ui::Node* layer = topLayer(state);

            if (state.focus != nullptr)
                return *state.focus;
            if (layer != nullptr)
                return *layer;
            return *state.root;
        }
    }

    // keyUp отменять нечего, но флаг ему не мешает / keyUp has nothing to cancel, but the flag does not hurt it
    bool fireKey(RouterState& state, const platform::Event& source)
    {
        ui::EventType type = ui::EventType::KEY_DOWN;

        if (source.kind == platform::EventKind::KEY_UP)
            type = ui::EventType::KEY_UP;
        ui::UiEvent event = ui::makeEvent(type, keyTarget(state));
        event.key = source.key;
        event.modifiers = source.modifiers;
        event.position = state.pointer;
        return fire(state, event);
    }

    /*
    ** Контекстное меню с клавиатуры: точка — центр цели в окне. Действие по умолчанию — встроенное меню
    ** у поля или выделяемого текста (как правый щелчок в центре).
    **
    ** The context menu from the keyboard: the point is the target center in the window. The default action is the built-in menu
    ** at a field or a selectable text (as a right click in the center).
    */
    bool contextKey(RouterState& state, const platform::Event& source)
    {
        bool shiftF10 = source.key == platform::Key::F10 && (source.modifiers & platform::MODIFIER_SHIFT) != 0;

        if (source.key != platform::Key::CONTEXT_MENU && !shiftF10)
            return false;
        ui::Node& target = keyTarget(state);
        Rect frame = affineBounds(ui::nodeToWindow(target), target.layout.frame);
        platform::Event press = source;
        press.kind = platform::EventKind::MOUSE_DOWN;
        press.button = platform::MouseButton::RIGHT;
        press.position = Vec2{frame.x + frame.width * 0.5f, frame.y + frame.height * 0.5f};
        ui::UiEvent event = pointerEvent(ui::EventType::CONTEXT_MENU, target, press);
        if (fire(state, event) && textTarget(&target, controlOf(&target)) != nullptr)
            contextClick(state, press);
        return true;
    }

    // Отмена перетаскивания важнее закрытия слоёв и снятия фокуса / Cancelling a drag beats closing the layers and removing the focus
    bool dragKey(RouterState& state, const platform::Event& source)
    {
        if (!state.nodeDrag.active || source.key != platform::Key::ESCAPE)
            return false;
        cancelDrag(state);
        return true;
    }
}
