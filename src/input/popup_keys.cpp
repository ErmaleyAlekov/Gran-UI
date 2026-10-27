#include "gran/input/input_router.hpp"

#include <vector>

namespace gran::input
{
    namespace
    {
        /*
        ** Стрелки вбок в меню: вправо на пункте с подменю — открыть его (фокус на первый пункт),
        ** влево в подменю — закрыть его и вернуться к пункту-родителю. false — стрелка не для меню
        ** (её получит поле под контекстным меню).
        **
        ** The side arrows in a menu: right on an item with a submenu — open it (the focus to the first item),
        ** left in a submenu — close it and return to the parent item. false — the arrow is not for the menu
        ** (the field under the context menu gets it).
        */
        bool submenuKey(RouterState& state, ui::Node& menu, platform::Key key)
        {
            ui::Node* focus = state.focus;
            ui::Node* owner = menu.parent;

            if (key == platform::Key::RIGHT && focus != nullptr && ui::popupOf(focus) == &menu
                && ui::submenuOf(*focus) != nullptr)
            {
                openSubmenu(state, *focus);
                return true;
            }
            if (key != platform::Key::LEFT || owner == nullptr || owner->kind != ui::ElementKind::MENU_ITEM)
                return false;
            closeLayer(state, menu);
            changeFocus(state, owner);
            return true;
        }
    }

    /*
    ** Следующий включённый пункт слоя по кругу на шаг step от пункта в фокусе; если фокус
    ** вне слоя — первый (вниз) или последний (вверх) пункт, как в меню Windows.
    **
    ** The next enabled item of the layer cyclically by step from the focused item; if the focus
    ** is outside the layer — the first (down) or the last (up) item, as in Windows menus.
    */
    ui::Node* stepItem(ui::Node& popup, const ui::Node* current, int step)
    {
        std::vector<ui::Node*> items;
        long position = -1;

        for (const std::unique_ptr<ui::Node>& child : popup.children)
        {
            if (child->kind == ui::ElementKind::MENU_ITEM && child->visible && child->enabled)
                items.push_back(child.get());
            if (child.get() == current)
                position = static_cast<long>(items.size()) - 1;
        }
        if (items.empty())
            return nullptr;
        long count = static_cast<long>(items.size());
        if (position < 0 && step > 0)
            return items.front();
        if (position < 0)
            return items.back();
        return items[static_cast<std::size_t>((position + step + count) % count)];
    }

    /*
    ** Esc закрывает верхний слой (диалог — если closable); стрелки работают только в меню:
    ** в диалоге они остаются полям и ползункам. Enter и пробел на пункте в фокусе обрабатывает
    ** общая активация (activate → chooseItem или подменю).
    **
    ** Esc closes the top layer (a dialog — if closable); the arrows work only in a menu:
    ** in a dialog they stay with the fields and sliders. Enter and space on the focused item are handled
    ** by the common activation (activate → chooseItem or a submenu).
    */
    bool popupKey(RouterState& state, const platform::Event& event)
    {
        ui::Node* popup = topLayer(state);
        int step = 1;

        if (popup == nullptr)
            return false;
        if (event.key == platform::Key::ESCAPE)
        {
            if (popup->kind == ui::ElementKind::POPUP || popup->content.closable)
                closeLayer(state, *popup);
            return true;
        }
        if (popup->kind != ui::ElementKind::POPUP)
            return false;
        if (event.key == platform::Key::LEFT || event.key == platform::Key::RIGHT)
            return submenuKey(state, *popup, event.key);
        if (event.key != platform::Key::UP && event.key != platform::Key::DOWN)
            return false;
        if (event.key == platform::Key::UP)
            step = -1;
        ui::Node* item = stepItem(*popup, state.focus, step);
        if (item != nullptr)
            changeFocus(state, item);
        return true;
    }
}
