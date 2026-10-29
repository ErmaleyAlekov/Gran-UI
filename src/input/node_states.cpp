#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Бит состояния одному узлу; изменение требует пересчёта стиля / A state bit to one node; a change requires a style recompute
    void setNodeState(ui::Node& node, std::uint8_t bit, bool on)
    {
        std::uint8_t mask = static_cast<std::uint8_t>(node.stateMask & ~bit);

        if (on)
            mask = static_cast<std::uint8_t>(mask | bit);
        if (mask == node.stateMask)
            return;
        node.stateMask = mask;
        ui::markDirty(node, ui::DIRTY_STYLE);
    }

    // От узла до корня / From the node up to the root
    void setChainState(ui::Node* node, std::uint8_t bit, bool on)
    {
        for (ui::Node* current = node; current != nullptr; current = current->parent)
            setNodeState(*current, bit, on);
    }

    // :hover — у узла под мышью и его предков; pointerLeave и pointerEnter — после обработки события
    // :hover — on the node under the mouse and its ancestors; pointerLeave and pointerEnter — after the event handling
    void changeHover(RouterState& state, ui::Node* node)
    {
        ui::Node* old = state.hover;

        if (old == node)
            return;
        setChainState(old, style::stateBit(style::State::HOVER), false);
        state.hover = node;
        setChainState(node, style::stateBit(style::State::HOVER), true);
        notifyHover(state, old, node);
    }

    // :active — у нажатого элемента и его предков / :active — on the pressed control and its ancestors
    void changePressed(RouterState& state, ui::Node* node)
    {
        if (state.pressed == node)
            return;
        setChainState(state.pressed, style::stateBit(style::State::PRESSED), false);
        state.pressed = node;
        setChainState(node, style::stateBit(style::State::PRESSED), true);
    }

    /*
    ** :focus — только у самого элемента; каретку и выделение ставят щелчок или Tab (text_pointer.cpp).
    ** blur и focus доставляются позже (notifyFocus): обработчик не застанет фокус на полпути.
    **
    ** :focus — only on the control itself; the caret and the selection are placed by a click or Tab (text_pointer.cpp).
    ** blur and focus are delivered later (notifyFocus): a handler does not catch the focus halfway.
    */
    void changeFocus(RouterState& state, ui::Node* node)
    {
        ui::Node* old = state.focus;

        if (old == node)
            return;
        if (old != nullptr)
            setNodeState(*old, style::stateBit(style::State::FOCUSED), false);
        state.focus = node;
        if (node != nullptr)
            setNodeState(*node, style::stateBit(style::State::FOCUSED), true);
        notifyFocus(state, old, node);
    }
}
