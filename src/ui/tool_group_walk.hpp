#pragma once
#include "gran/ui/node.hpp"

namespace gran::ui::internal
{
    inline const std::string& overflowLabel(const Node& node) noexcept
    {
        if (!node.content.text.empty())
            return node.content.text;
        return node.content.tooltip.empty() ? node.content.accessibleName : node.content.tooltip;
    }

    template<class Visit>
    void visitToolItems(const Node& root, bool shown, Visit visit)
    {
        struct Item { const Node* node; bool shown; };
        std::vector<Item> pending{{&root, shown}};
        while (!pending.empty())
        {
            const auto item = pending.back();
            pending.pop_back();
            if (item.node->kind != ElementKind::TOOL_GROUP)
                visit(*item.node, item.shown);
            else
                for (auto it = item.node->children.rbegin(); it != item.node->children.rend(); ++it)
                    pending.push_back({it->get(), item.shown && (*it)->visible});
        }
    }
}
