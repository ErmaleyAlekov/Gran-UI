#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Подъём от узла к корню: встретили subtree — узел внутри / Climbing from the node to the root: meeting subtree means the node is inside
    bool within(const ui::Node* node, const ui::Node& subtree) noexcept
    {
        for (const ui::Node* current = node; current != nullptr; current = current->parent)
        {
            if (current == &subtree)
                return true;
        }
        return false;
    }

    // Стек просматривается сверху вниз / The stack is looked through top down
    ui::Node* topLayer(const RouterState& state) noexcept
    {
        for (std::size_t index = state.layers.size(); index > 0; --index)
        {
            if (!state.layers[index - 1].node->content.popupPassive)
                return state.layers[index - 1].node;
        }
        return nullptr;
    }

    // Меню и подсказки над диалогом модальность не снимают: ищется верхний диалог, а не верхний слой
    // Menus and tooltips over a dialog do not lift the modality: the top dialog is searched, not the top layer
    ui::Node* modalLayer(const RouterState& state) noexcept
    {
        for (std::size_t index = state.layers.size(); index > 0; --index)
        {
            if (state.layers[index - 1].node->kind == ui::ElementKind::DIALOG)
                return state.layers[index - 1].node;
        }
        return nullptr;
    }

    /*
    ** Вложенные слои закрываются по одному, каждый раз верхний из оставшихся: закрытие слоя
    ** меняет стек (и закрывает его собственные вложенные), поэтому поиск начинается заново.
    ** Слоёв единицы, квадратичный поиск дешевле лишней структуры.
    **
    ** The nested layers close one by one, each time the top one of the remaining: closing a layer
    ** changes the stack (and closes its own nested ones), so the search starts anew.
    ** There are a few layers, the quadratic search is cheaper than an extra structure.
    */
    void closeNested(RouterState& state, const ui::Node& layer)
    {
        ui::Node* victim = nullptr;

        do
        {
            victim = nullptr;
            for (const OpenLayer& open : state.layers)
            {
                if (open.node != &layer && within(open.node, layer))
                    victim = open.node;
            }
            if (victim != nullptr)
                closeLayer(state, *victim);
        } while (victim != nullptr);
    }

    // Меню (Popup) закрываются сверху, пока не встретится диалог: выбор в меню диалога его не закрывает
    // The menus (Popup) close from the top until a dialog is met: a choice in a dialog menu does not close it
    void closeMenus(RouterState& state)
    {
        ui::Node* top = topLayer(state);

        while (top != nullptr && top->kind == ui::ElementKind::POPUP)
        {
            closeLayer(state, *top);
            top = topLayer(state);
        }
    }
}
