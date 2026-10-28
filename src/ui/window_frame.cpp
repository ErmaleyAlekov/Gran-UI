#include "gran/ui/node.hpp"

namespace gran::ui
{
    // Команды окна доступны любому виду элемента, в том числе контейнеру со значком.
    // Window commands are available to any element kind, including a container with an icon.
    bool isWindowButton(const Node& node) noexcept
    {
        platform::WindowRole role = node.content.windowRole;

        return role == platform::WindowRole::MINIMIZE || role == platform::WindowRole::MAXIMIZE
            || role == platform::WindowRole::CLOSE;
    }

    // Бит распространяется на детей: значок внутри кнопки тоже может сменить оформление.
    // Propagate the bit to children: an icon inside a button can also change its appearance.
    void setWindowMaximized(Node& node, bool maximized)
    {
        std::uint8_t bit = style::stateBit(style::State::MAXIMIZED);

        if (((node.stateMask & bit) != 0) == maximized && (node.dirty & (DIRTY_STYLE | DIRTY_DESCENDANT)) == 0)
            return;
        if (((node.stateMask & bit) != 0) != maximized)
        {
            node.stateMask ^= bit;
            markDirty(node, DIRTY_STYLE | DIRTY_PAINT);
        }
        for (const std::unique_ptr<Node>& child : node.children)
            setWindowMaximized(*child, maximized);
    }
}
