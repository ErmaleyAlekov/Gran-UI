#include "gran/ui/node.hpp"

namespace gran::ui
{
    // Оба вида правятся одним кодом ввода; различаются переносами и клавишей Enter
    // Both kinds are edited by the same input code; they differ in line breaks and the Enter key
    bool isTextField(const Node& node) noexcept
    {
        return node.kind == ElementKind::INPUT || node.kind == ElementKind::TEXT_AREA;
    }

    // Слой вынут из потока, как position: fixed в CSS: его место задаёт якорь или окно, а не родитель
    // A layer is taken out of the flow, as position: fixed in CSS: its place is set by the anchor or the window, not by the parent
    bool inFlow(const Node& node) noexcept
    {
        return node.visible && !isLayer(node);
    }

    // Подъём к корню до первого слоя / Climbing to the root up to the first layer
    Node* popupOf(Node* node) noexcept
    {
        for (Node* current = node; current != nullptr; current = current->parent)
        {
            if (isLayer(*current))
                return current;
        }
        return nullptr;
    }

    // Диалог — тот же слой, что и меню, но модальный и по центру окна / A dialog is the same layer as a menu, but modal and centered in the window
    bool isLayer(const Node& node) noexcept
    {
        return node.kind == ElementKind::POPUP || node.kind == ElementKind::DIALOG;
    }

    // Состояние DISABLED меняет вид (стиль Disabled), поэтому узел перерисовывается со стилем
    // The DISABLED state changes the look (the Disabled style), so the node is restyled and repainted
    void setEnabled(Node& node, bool enabled)
    {
        std::uint8_t bit = style::stateBit(style::State::DISABLED);

        if (node.enabled == enabled)
            return;
        node.enabled = enabled;
        if (enabled)
            node.stateMask = static_cast<std::uint8_t>(node.stateMask & ~bit);
        else
            node.stateMask = static_cast<std::uint8_t>(node.stateMask | bit);
        markDirty(node, DIRTY_STYLE | DIRTY_PAINT);
    }
}
