#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Открыт ли хоть один непассивный слой-меню (не диалог) / Whether at least one non-passive menu layer (not a dialog) is open
        bool menuOpen(const RouterState& state) noexcept
        {
            for (const OpenLayer& open : state.layers)
            {
                if (open.node->kind == ui::ElementKind::POPUP && !open.node->content.popupPassive)
                    return true;
            }
            return false;
        }
    }

    /*
    ** Владелец слоя — пункт меню (подменю, меню строки) или ComboBox (список): пока слой открыт, у владельца
    ** состояние Open — пункт строки меню и пункт-родитель подменю остаются подсвеченными.
    **
    ** The layer owner is a menu item (a submenu, a bar menu) or a ComboBox (the list): while the layer is open the owner
    ** has the Open state — the menu bar item and the submenu parent item stay highlighted.
    */
    void markOpen(ui::Node& layer, bool open)
    {
        ui::Node* owner = layer.parent;

        if (owner == nullptr || (owner->kind != ui::ElementKind::MENU_ITEM && owner->kind != ui::ElementKind::COMBO_BOX))
            return;
        setNodeState(*owner, style::stateBit(style::State::OPEN), open);
    }

    /*
    ** До обработки: нажатие мыши разряжает Alt и прячет подсказки клавиатуры (меню дальше ведёт мышь), потеря
    ** фокуса окном отпускает Alt (его отпускание придёт уже другому окну). После: меню с клавиатуры закрылись и
    ** фокус ушёл из строки — подсказки гаснут (если ОС не просит показывать их всегда); смена видимости
    ** подчёркиваний перерисовывает окно.
    **
    ** Before the handling: a mouse press disarms Alt and hides the keyboard cues (the mouse leads the menus further), the window
    ** losing focus releases Alt (its release will come to another window). After: the keyboard menus closed and the focus
    ** left the bar — the cues go out (unless the OS asks to show them always); a change of the underline visibility
    ** repaints the window.
    */
    bool trackMenuCues(RouterState& state, const platform::Event& event, bool after)
    {
        if (!after)
        {
            if (event.kind == platform::EventKind::MOUSE_DOWN || event.kind == platform::EventKind::FOCUS_LOST)
            {
                state.altArmed = false;
                state.keyboardMenus = false;
            }
            if (event.kind == platform::EventKind::FOCUS_LOST)
                state.altHeld = false;
            return false;
        }
        if (state.keyboardMenus && !menuOpen(state) && (state.focus == nullptr || !ui::isMenuBarItem(*state.focus)))
            state.keyboardMenus = false;
        bool wanted = state.altHeld || state.keyboardMenus
            || (state.window != nullptr && state.window->keyboardCuesAlways());
        if (wanted == state.cuesShown)
            return false;
        state.cuesShown = wanted;
        ui::markDirty(*state.root, ui::DIRTY_PAINT);
        return true;
    }

    // Читает кадр: подчёркивать ли букву доступа у пунктов меню / The frame reads it: whether to underline the access letter of the menu items
    bool InputRouter::mnemonicsShown() const noexcept
    {
        return _state.cuesShown;
    }
}
