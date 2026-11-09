#include "gran/input/input_router.hpp"

namespace gran::input
{
    /*
    ** Строка меню с фокусом и без открытого меню (как в Windows после Alt): стрелки вбок ходят по пунктам
    ** по кругу, вниз, вверх, Enter и пробел открывают меню пункта, Esc возвращает фокус, Tab поглощается.
    **
    ** The menu bar with the focus and without an open menu (as in Windows after Alt): the side arrows walk the items
    ** cyclically, down, up, Enter and space open the item menu, Esc returns the focus, Tab is consumed.
    */
    bool menuBarKey(RouterState& state, const platform::Event& event)
    {
        ui::Node* item = state.focus;
        platform::Key key = event.key;
        int step = 1;

        if (item == nullptr || !ui::isMenuBarItem(*item) || topLayer(state) != nullptr)
            return false;
        if (key == platform::Key::LEFT || key == platform::Key::RIGHT)
        {
            if (key == platform::Key::LEFT)
                step = -1;
            ui::Node* next = stepItem(*item->parent, item, step);
            if (next != nullptr)
                changeFocus(state, next);
            return true;
        }
        if (key == platform::Key::DOWN || key == platform::Key::UP || key == platform::Key::ENTER || key == platform::Key::SPACE)
            enterMenuBar(state, *item, true);
        else if (key == platform::Key::ESCAPE)
            leaveMenuBar(state);
        else
            return key == platform::Key::TAB;
        return true;
    }

    /*
    ** В меню пункта строки стрелка влево — меню соседа слева, вправо — справа (если пункт в фокусе не открывает
    ** своё подменю). Сосед без меню (пункт-команда) получает только фокус: стрелка не должна выполнять команду.
    **
    ** In the menu of a bar item the left arrow is the left neighbor menu, the right one — the right neighbor (if the focused item
    ** does not open its own submenu). A neighbor without a menu (a command item) gets the focus only: an arrow must not run a command.
    */
    bool barSideKey(RouterState& state, ui::Node& menu, platform::Key key)
    {
        ui::Node* owner = menu.parent;
        int step = 1;

        if (owner == nullptr || !ui::isMenuBarItem(*owner) || (key != platform::Key::LEFT && key != platform::Key::RIGHT))
            return false;
        if (key == platform::Key::RIGHT && state.focus != nullptr && ui::popupOf(state.focus) == &menu
            && ui::submenuOf(*state.focus) != nullptr)
            return false;
        if (key == platform::Key::LEFT)
            step = -1;
        ui::Node* next = stepItem(*owner->parent, owner, step);
        closeLayer(state, menu);
        if (next != nullptr)
            enterMenuBar(state, *next, ui::submenuOf(*next) != nullptr);
        return true;
    }

    /*
    ** Пока открыто меню одного пункта строки, наведение на другой пункт закрывает это меню и открывает его
    ** (как в строке меню Windows); фокус в строке (вход с клавиатуры) идёт за мышью.
    **
    ** While the menu of one bar item is open, hovering another item closes that menu and opens its one
    ** (as in the Windows menu bar); the focus in the bar (a keyboard entry) follows the mouse.
    */
    void hoverMenuBar(RouterState& state, ui::Node* hover)
    {
        ui::Node* item = controlOf(hover);
        ui::Node* menu = openBarMenu(state);

        if (item == nullptr || menu == nullptr || !ui::isMenuBarItem(*item) || menu->parent == item || disabled(item))
            return;
        closeLayer(state, *menu);
        if (state.focus != nullptr && ui::isMenuBarItem(*state.focus))
            changeFocus(state, item);
        if (ui::submenuOf(*item) != nullptr)
            openSubmenu(state, *item);
    }

    /*
    ** Открытое меню (не список ComboBox): буква выбирает пункт (с подменю — открывает его), чужие буквы
    ** поглощаются, пока фокус в меню. Иначе буква с Alt — или без него, когда фокус в строке меню, — открывает
    ** меню пункта строки; Alt+буква никогда не печатается в поле, даже если такой буквы нет.
    **
    ** An open menu (not a ComboBox list): a letter chooses an item (with a submenu — opens it), other letters
    ** are consumed while the focus is in the menu. Otherwise a letter with Alt — or without it when the focus is in the menu bar —
    ** opens the bar item menu; Alt+letter is never typed into a field, even if there is no such letter.
    */
    bool mnemonicKey(RouterState& state, const platform::Event& event)
    {
        bool alt = (event.modifiers & platform::MODIFIER_ALT) != 0 && (event.modifiers & platform::MODIFIER_CONTROL) == 0;
        ui::Node* layer = topLayer(state);
        ui::Node* bar = nullptr;

        if (layer != nullptr && layer->kind == ui::ElementKind::POPUP
            && (layer->parent == nullptr || layer->parent->kind != ui::ElementKind::COMBO_BOX))
        {
            ui::Node* item = ui::mnemonicItem(*layer, event.character);
            if (item == nullptr)
                return alt || within(state.focus, *layer);
            state.keyboard = true;
            activate(state, *item);
            return true;
        }
        if (!alt && (state.focus == nullptr || !ui::isMenuBarItem(*state.focus)))
            return false;
        if (modalLayer(state) == nullptr)
            bar = ui::menuBarOf(*state.root);
        ui::Node* item = nullptr;
        if (bar != nullptr)
            item = ui::mnemonicItem(*bar, event.character);
        if (item != nullptr)
        {
            closeMenus(state);
            enterMenuBar(state, *item, true);
        }
        return true;
    }

    // Слой, чей владелец — пункт строки меню / The layer whose owner is a menu bar item
    ui::Node* openBarMenu(const RouterState& state) noexcept
    {
        for (const OpenLayer& open : state.layers)
        {
            if (open.node->parent != nullptr && ui::isMenuBarItem(*open.node->parent))
                return open.node;
        }
        return nullptr;
    }
}
