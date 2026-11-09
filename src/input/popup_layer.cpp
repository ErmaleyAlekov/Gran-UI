#include "gran/input/input_router.hpp"

#include <vector>

namespace gran::input
{
    /*
    ** Слои проверяются сверху вниз; пассивный (подсказка) мышь пропускает к тому, что под ним.
    ** Диалог модальный: мимо него точка не попадает никуда — ни в дерево, ни в слои ниже.
    **
    ** The layers are checked top down; a passive one (a tooltip) lets the mouse through to what is under it.
    ** A dialog is modal: a point outside it hits nothing — neither the tree nor the layers below.
    */
    ui::Node* pickNode(RouterState& state, Vec2 point)
    {
        for (std::size_t index = state.layers.size(); index > 0; --index)
        {
            ui::Node& layer = *state.layers[index - 1].node;
            if (layer.content.popupPassive)
                continue;
            ui::Node* hit = hitLayer(layer, point);
            if (hit != nullptr)
                return hit;
            if (layer.kind == ui::ElementKind::DIALOG)
                return nullptr;
        }
        return hitTest(*state.root, point);
    }

    /*
    ** Слой становится видимым; новый стиль и компоновка — на ближайшем кадре. Фокус до открытия
    ** запоминается в записи слоя (повторное открытие сохраняет прежний: фокус уже может быть внутри),
    ** затем слой сам ставит фокус (focusOnOpen).
    **
    ** The layer becomes visible; a new style and layout — in the nearest frame. The focus before opening
    ** is remembered in the layer entry (opening again keeps the former one: the focus may already be inside),
    ** then the layer places the focus itself (focusOnOpen).
    */
    void openLayer(RouterState& state, ui::Node& popup, const Rect& anchor, bool passive)
    {
        OpenLayer entry{&popup, state.focus};

        for (std::size_t index = 0; index < state.layers.size(); ++index)
        {
            if (state.layers[index].node != &popup)
                continue;
            entry.returnFocus = state.layers[index].returnFocus;
            state.layers.erase(state.layers.begin() + static_cast<std::ptrdiff_t>(index));
            break;
        }
        if (passive)
            entry.returnFocus = nullptr;
        state.layers.push_back(entry);
        popup.content.popupAnchor = anchor;
        popup.content.popupPassive = passive;
        popup.visible = true;
        markOpen(popup, true);
        ui::markDirty(popup, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        if (!passive)
            focusOnOpen(state, popup);
    }

    /*
    ** Сначала закрываются вложенные слои (их фокус возвращается сюда же), затем сам слой: наведение
    ** и нажатие внутри снимаются, фокус изнутри (или потерянный) — туда, где был до открытия,
    ** если тот узел ещё виден.
    **
    ** The nested layers close first (their focus returns into this one), then the layer itself: the hover
    ** and the press inside are dropped, the focus from inside (or a lost one) goes where it was before opening,
    ** if that node is still shown.
    */
    void closeLayer(RouterState& state, ui::Node& popup)
    {
        ui::Node* returnTo = nullptr;

        closeNested(state, popup);
        for (std::size_t index = 0; index < state.layers.size(); ++index)
        {
            if (state.layers[index].node != &popup)
                continue;
            returnTo = state.layers[index].returnFocus;
            state.layers.erase(state.layers.begin() + static_cast<std::ptrdiff_t>(index));
            break;
        }
        popup.visible = false;
        markOpen(popup, false);
        ui::markDirty(popup, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        if (within(state.hover, popup))
            changeHover(state, nullptr);
        if (within(state.pressed, popup))
            changePressed(state, nullptr);
        if (state.focus != nullptr && !within(state.focus, popup))
            return;
        if (returnTo != nullptr && !ui::isShown(*returnTo))
            returnTo = nullptr;
        changeFocus(state, returnTo);
    }

    /*
    ** Щелчок мимо закрывает непассивные слои над тем, в который он попал (как меню Windows),
    ** и сам никуда не идёт: иначе нажатие «сквозь» меню нечаянно нажало бы кнопку под ним.
    ** Диалог мимо себя не закрывается и щелчок поглощает (модальность).
    **
    ** A click outside closes the non-passive layers over the one it hit (as Windows menus)
    ** and goes nowhere itself: otherwise a press "through" a menu would accidentally press a button under it.
    ** A dialog is not closed by a click outside it and consumes the click (modality).
    */
    bool closeOutside(RouterState& state, Vec2 point)
    {
        std::vector<ui::Node*> above;
        bool blocked = false;

        for (std::size_t index = state.layers.size(); index > 0 && !blocked; --index)
        {
            ui::Node& layer = *state.layers[index - 1].node;
            if (layer.content.popupPassive)
                continue;
            if (hitLayer(layer, point) != nullptr)
                break;
            blocked = layer.kind == ui::ElementKind::DIALOG;
            if (!blocked)
                above.push_back(&layer);
        }
        for (ui::Node* layer : above)
            closeLayer(state, *layer);
        return blocked || !above.empty();
    }

    /*
    ** Цепочка меню закрывается первой: команда видит фокус на прежнем месте и может открыть другой
    ** слой. Пункт списка ComboBox меняет его значение (onChange — только при смене), затем onClick пункта.
    **
    ** The menu chain closes first: the command sees the focus in its former place and may open another
    ** layer. A ComboBox list item changes its value (onChange — only on a change), then the item onClick.
    */
    void chooseItem(RouterState& state, ui::Node& item)
    {
        ui::Node* list = ui::popupOf(&item);
        ui::Node* combo = nullptr;

        if (list != nullptr && list->parent != nullptr && list->parent->kind == ui::ElementKind::COMBO_BOX)
            combo = list->parent;
        closeMenus(state);
        if (list != nullptr && list->kind == ui::ElementKind::POPUP && list->visible)
            closeLayer(state, *list);
        // Выбор из меню строки меню: фокус возвращается туда, где был до входа в строку / A choice from a bar menu: the focus returns where it was before entering the bar
        if (state.focus != nullptr && ui::isMenuBarItem(*state.focus))
            leaveMenuBar(state);
        if (runContextCommand(state, item))
            return;
        if (combo != nullptr && ui::selectOption(*combo, &item) && !combo->content.onChange.empty())
            state.actions->action(combo->content.onChange, *combo);
        if (!item.content.command.empty())
            state.actions->command(item.content.command, item);
        if (!item.content.onClick.empty())
            state.actions->action(item.content.onClick, item);
    }
}
