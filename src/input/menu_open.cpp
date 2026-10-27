#include "gran/input/input_router.hpp"

#include <vector>

namespace gran::input
{
    namespace
    {
        /*
        ** Слой элемента: у ComboBox — его внутренний список, у кнопки — узел по id из popup
        ** (только Popup или Dialog; неизвестный id — ничего, как onClick без обработчика).
        **
        ** The control layer: for a ComboBox — its inner list, for a button — the node by the id from popup
        ** (only a Popup or a Dialog; an unknown id — nothing, as onClick without a handler).
        */
        ui::Node* attachedLayer(RouterState& state, ui::Node& control)
        {
            if (control.kind == ui::ElementKind::COMBO_BOX)
                return ui::comboList(control);
            if (control.content.popup.empty())
                return nullptr;
            ui::Node* layer = ui::findNode(*state.root, control.content.popup);
            if (layer == nullptr || !ui::isLayer(*layer))
                return nullptr;
            return layer;
        }
    }

    // Якорь — описанный прямоугольник элемента в координатах окна (диалогу он не нужен) / The anchor is the control bounding rectangle in window coordinates (a dialog does not need it)
    bool openAttached(RouterState& state, ui::Node& control)
    {
        ui::Node* layer = attachedLayer(state, control);

        if (layer == nullptr || disabled(&control))
            return false;
        openLayer(state, *layer, affineBounds(ui::nodeToWindow(control), control.layout.frame), false);
        return true;
    }

    // Место подменю считает компоновка по рамке пункта; якорь хранится для единообразия / The layout computes the submenu place by the item frame; the anchor is kept for uniformity
    void openSubmenu(RouterState& state, ui::Node& item)
    {
        ui::Node* submenu = ui::submenuOf(item);

        if (submenu == nullptr || disabled(&item))
            return;
        openLayer(state, *submenu, item.layout.frame, false);
    }

    /*
    ** Подменю открывается наведением сразу, без задержки: оно встаёт вплотную справа, и путь
    ** мыши к нему не задевает соседние пункты. Закрываются слои внутри меню пункта, кроме его подменю.
    **
    ** A submenu opens on hover at once, without a delay: it stands right next to the item, and the mouse
    ** path to it does not touch the neighbor items. The layers inside the item menu close, except its submenu.
    */
    void hoverMenus(RouterState& state, ui::Node* hover)
    {
        ui::Node* item = controlOf(hover);
        std::vector<ui::Node*> stale;

        if (item == nullptr || item->kind != ui::ElementKind::MENU_ITEM || ui::popupOf(item) == nullptr)
            return;
        ui::Node* menu = ui::popupOf(item);
        ui::Node* submenu = ui::submenuOf(*item);
        for (const OpenLayer& open : state.layers)
        {
            if (open.node != menu && open.node != submenu && within(open.node, *menu))
                stale.push_back(open.node);
        }
        for (ui::Node* layer : stale)
            closeLayer(state, *layer);
        if (submenu != nullptr && !submenu->visible)
            openSubmenu(state, *item);
    }

    /*
    ** Диалог забирает фокус всегда (как showModal в HTML): ввод с клавиатуры не должен уходить
    ** под затемнение; поле получает каретку без выделения. Меню не трогает выделение: «Копировать»
    ** берёт именно его.
    **
    ** A dialog always takes the focus (as showModal in HTML): the keyboard input must not go
    ** under the dimming; a field gets the caret without a selection. A menu does not touch the selection: "Copy"
    ** takes exactly it.
    */
    void focusOnOpen(RouterState& state, ui::Node& layer)
    {
        ui::Node* target = nullptr;

        if (layer.kind == ui::ElementKind::DIALOG)
        {
            target = nextFocusable(layer, nullptr, false);
            clearSelection(state);
            changeFocus(state, target);
            if (target != nullptr && ui::isTextField(*target))
                state.selection = target;
            return;
        }
        if (layer.parent != nullptr && layer.parent->kind == ui::ElementKind::COMBO_BOX)
            target = ui::findOption(*layer.parent, layer.parent->content.option);
        if (target == nullptr && state.keyboard)
            target = stepItem(layer, nullptr, 1);
        if (target != nullptr)
            changeFocus(state, target);
    }
}
