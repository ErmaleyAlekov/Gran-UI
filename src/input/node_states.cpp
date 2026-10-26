#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Бит состояния одному узлу; изменение требует пересчёта стиля / A state bit to one node; a change requires a style recompute
        void setState(ui::Node& node, std::uint8_t bit, bool on)
        {
            std::uint8_t mask = static_cast<std::uint8_t>(node.stateMask & ~bit);

            if (on)
                mask = static_cast<std::uint8_t>(mask | bit);
            if (mask == node.stateMask)
                return;
            node.stateMask = mask;
            ui::markDirty(node, ui::DIRTY_STYLE);
        }
    }

    // От узла до корня / From the node up to the root
    void setChainState(ui::Node* node, std::uint8_t bit, bool on)
    {
        for (ui::Node* current = node; current != nullptr; current = current->parent)
            setState(*current, bit, on);
    }

    // :hover — у узла под мышью и его предков / :hover — on the node under the mouse and its ancestors
    void changeHover(RouterState& state, ui::Node* node)
    {
        if (state.hover == node)
            return;
        setChainState(state.hover, style::stateBit(style::State::HOVER), false);
        state.hover = node;
        setChainState(node, style::stateBit(style::State::HOVER), true);
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

    // :focus — только у самого элемента; каретку и выделение ставят щелчок или Tab (text_pointer.cpp)
    // :focus — only on the control itself; the caret and the selection are placed by a click or Tab (text_pointer.cpp)
    void changeFocus(RouterState& state, ui::Node* node)
    {
        if (state.focus == node)
            return;
        if (state.focus != nullptr)
            setState(*state.focus, style::stateBit(style::State::FOCUSED), false);
        state.focus = node;
        if (node == nullptr)
            return;
        setState(*node, style::stateBit(style::State::FOCUSED), true);
    }
}
