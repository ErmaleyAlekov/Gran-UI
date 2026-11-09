#include "gran/input/input_router.hpp"

#include <vector>

namespace gran::input
{
    namespace
    {
        /*
        ** Alt или F10: строка меню активна (фокус на ней или открыто её меню) — выйти; иначе — войти на первый
        ** пункт. Под открытым диалогом строки меню нет: ввод ниже диалога заблокирован.
        **
        ** Alt or F10: the menu bar is active (the focus is on it or its menu is open) — leave; otherwise enter at the first
        ** item. Under an open dialog there is no menu bar: the input below the dialog is blocked.
        */
        void toggleMenuBar(RouterState& state)
        {
            ui::Node* bar = ui::menuBarOf(*state.root);

            if (bar == nullptr || modalLayer(state) != nullptr)
                return;
            if (openBarMenu(state) != nullptr || (state.focus != nullptr && ui::isMenuBarItem(*state.focus)))
            {
                leaveMenuBar(state);
                return;
            }
            ui::Node* first = stepItem(*bar, nullptr, 1);
            if (first == nullptr)
                return;
            closeMenus(state);
            enterMenuBar(state, *first, false);
        }
    }

    /*
    ** Прежний фокус запоминается, только если он вне строки (переход между пунктами его не теряет).
    ** Меню пункта открывается как с клавиатуры — фокус на первый пункт. Пункт без меню (с командой)
    ** выполняется после выхода из строки: команда видит фокус на прежнем месте, как у chooseItem.
    **
    ** The former focus is remembered only if it is outside the bar (moving between the items does not lose it).
    ** The item menu opens as from the keyboard — the focus goes to the first item. An item without a menu (with a command)
    ** runs after leaving the bar: the command sees the focus in its former place, as with chooseItem.
    */
    void enterMenuBar(RouterState& state, ui::Node& item, bool open)
    {
        if (state.focus == nullptr || !ui::isMenuBarItem(*state.focus))
            state.barReturn = state.focus;
        state.keyboardMenus = true;
        if (open && ui::submenuOf(item) == nullptr)
        {
            leaveMenuBar(state);
            chooseItem(state, item);
            return;
        }
        changeFocus(state, &item);
        if (!open)
            return;
        state.keyboard = true;
        openSubmenu(state, item);
    }

    /*
    ** Закрываются меню пунктов строки (и всё вложенное в них); фокус с пункта строки уходит туда, где был до
    ** входа, если тот узел ещё виден. Без входа с клавиатуры (меню открыли мышью) фокус и так не в строке.
    **
    ** The menus of the bar items close (and everything nested in them); the focus leaves a bar item for where it was
    ** before entering, if that node is still shown. Without a keyboard entry (the menu was opened by the mouse) the focus is not in the bar anyway.
    */
    void leaveMenuBar(RouterState& state)
    {
        ui::Node* menu = openBarMenu(state);
        ui::Node* back = state.barReturn;

        if (menu != nullptr)
            closeLayer(state, *menu);
        if (back != nullptr && !ui::isShown(*back))
            back = nullptr;
        if (state.focus != nullptr && ui::isMenuBarItem(*state.focus))
            changeFocus(state, back);
        state.barReturn = nullptr;
        state.keyboardMenus = false;
    }

    /*
    ** Alt взводится только без Ctrl и Shift (AltGr в Windows — это Ctrl+Alt, Shift+Alt — смена раскладки) и
    ** разряжается любой другой клавишей: отпускание взведённого Alt входит в строку меню или выходит из неё.
    ** F10 без модификаторов делает то же сразу (Shift+F10 — контекстное меню).
    **
    ** Alt is armed only without Ctrl and Shift (AltGr on Windows is Ctrl+Alt, Shift+Alt switches the layout) and
    ** is disarmed by any other key: releasing an armed Alt enters the menu bar or leaves it.
    ** F10 without modifiers does the same at once (Shift+F10 is the context menu).
    */
    bool altKey(RouterState& state, const platform::Event& event)
    {
        bool down = event.kind == platform::EventKind::KEY_DOWN;
        std::uint8_t others = platform::MODIFIER_CONTROL | platform::MODIFIER_SHIFT;

        if (event.key == platform::Key::F10 && down && event.modifiers == 0)
        {
            toggleMenuBar(state);
            return true;
        }
        if (event.key != platform::Key::ALT)
        {
            state.altArmed = state.altArmed && !down;
            return false;
        }
        // Автоповтор зажатого Alt не взводит его заново после разрядки / The auto-repeat of a held Alt does not re-arm it after disarming
        if (down && !state.altHeld)
            state.altArmed = (event.modifiers & others) == 0;
        state.altHeld = down;
        if (down)
            return false;
        if (state.altArmed)
            toggleMenuBar(state);
        state.altArmed = false;
        return true;
    }
}
