#include "gran/input/input_router.hpp"

#include <algorithm>

namespace gran::input
{
    // Слои проверяются сверху вниз; пассивный (подсказка) мышь пропускает к тому, что под ним
    // The layers are checked top down; a passive one (a tooltip) lets the mouse through to what is under it
    ui::Node* pickNode(RouterState& state, Vec2 point)
    {
        for (std::size_t index = state.popups.size(); index > 0; --index)
        {
            ui::Node& popup = *state.popups[index - 1];
            if (popup.content.popupPassive)
                continue;
            ui::Node* hit = hitLayer(popup, point);
            if (hit != nullptr)
                return hit;
        }
        return hitTest(*state.root, point);
    }

    /*
    ** Слой становится видимым у якоря; новый стиль и компоновка — на ближайшем кадре. Фокус до
    ** открытия запоминается для первого непассивного слоя: в него он вернётся при закрытии.
    **
    ** The layer becomes visible at the anchor; a new style and layout — in the nearest frame. The focus before
    ** opening is remembered for the first non-passive layer: it returns there on closing.
    */
    void openLayer(RouterState& state, ui::Node& popup, const Rect& anchor, bool passive)
    {
        std::vector<ui::Node*>& popups = state.popups;

        popups.erase(std::remove(popups.begin(), popups.end(), &popup), popups.end());
        if (!passive && ui::popupOf(state.focus) == nullptr)
            state.returnFocus = state.focus;
        popups.push_back(&popup);
        popup.content.popupAnchor = anchor;
        popup.content.popupPassive = passive;
        popup.visible = true;
        ui::markDirty(popup, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }

    // Наведение и нажатие внутри слоя снимаются; фокус из слоя — обратно / The hover and the press inside the layer are dropped; the focus from the layer goes back
    void closeLayer(RouterState& state, ui::Node& popup)
    {
        std::vector<ui::Node*>& popups = state.popups;

        popups.erase(std::remove(popups.begin(), popups.end(), &popup), popups.end());
        popup.visible = false;
        ui::markDirty(popup, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        if (ui::popupOf(state.hover) == &popup)
            changeHover(state, nullptr);
        if (ui::popupOf(state.pressed) == &popup)
            changePressed(state, nullptr);
        if (ui::popupOf(state.focus) == &popup)
            changeFocus(state, state.returnFocus);
    }

    /*
    ** Щелчок мимо меню закрывает все непассивные слои и сам никуда не идёт (как меню Windows):
    ** иначе нажатие «сквозь» меню нечаянно нажало бы кнопку под ним.
    **
    ** A click outside a menu closes all non-passive layers and goes nowhere itself (as Windows menus):
    ** otherwise a press "through" the menu would accidentally press a button under it.
    */
    bool closeOutside(RouterState& state, Vec2 point)
    {
        std::vector<ui::Node*> open;

        for (ui::Node* popup : state.popups)
        {
            if (popup->content.popupPassive)
                continue;
            if (hitLayer(*popup, point) != nullptr)
                return false;
            open.push_back(popup);
        }
        for (ui::Node* popup : open)
            closeLayer(state, *popup);
        return !open.empty();
    }

    // Слой закрывается первым: команда видит фокус на прежнем месте и может открыть другой слой
    // The layer closes first: the command sees the focus in its former place and may open another layer
    void chooseItem(RouterState& state, ui::Node& item)
    {
        ui::Node* popup = ui::popupOf(&item);

        if (popup != nullptr)
            closeLayer(state, *popup);
        if (runContextCommand(state, item))
            return;
        if (!item.content.onClick.empty())
            state.actions->action(item.content.onClick, item);
    }
}
