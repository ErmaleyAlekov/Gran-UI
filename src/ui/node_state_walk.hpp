#ifndef GRAN_UI_NODE_STATE_WALK_HPP
#define GRAN_UI_NODE_STATE_WALK_HPP

#include "gran/ui/node_state.hpp"

namespace gran::ui::internal
{
    using IdCounts = std::map<std::string, std::size_t>;

    inline void countStateIds(const Node& node, IdCounts& counts)
    {
        if (!node.id.empty())
            ++counts[node.id];
        for (const auto& child : node.children)
            countStateIds(*child, counts);
    }

    inline std::string stateId(const std::string& id)
    {
        std::string key = "#";
        for (char c : id)
        {
            if (c == '/' || c == '%')
                key += c == '/' ? "%2F" : "%25";
            else
                key += c;
        }
        return key;
    }

    // Путь различает одинаковые id по ветви и порядку среди одноимённых соседей.
    // The path distinguishes repeated IDs by branch and occurrence among siblings with the same ID.
    template<class N, class Visitor>
    void walkStateNodes(N& node, const IdCounts& counts, const std::string& path, std::string key, Visitor& visit)
    {
        visit(node, key, path);
        IdCounts siblings;
        for (std::size_t index = 0; index < node.children.size(); ++index)
        {
            N& child = *node.children[index];
            std::string slot = "/" + std::to_string(index);
            std::string step = slot;
            if (!child.id.empty())
                step = "/" + stateId(child.id) + "/" + std::to_string(siblings[child.id]++);
            std::string childKey = key + slot;
            if (!child.id.empty())
            {
                if (counts.at(child.id) == 1)
                    childKey = stateId(child.id);
                else
                    childKey = (key.compare(0, 1, "@") == 0 ? key : "@" + key) + step;
            }
            walkStateNodes(child, counts, path + step, childKey, visit);
        }
    }

    template<class N, class Visitor>
    void walkState(N& root, Visitor visit)
    {
        IdCounts counts;
        countStateIds(root, counts);
        const auto rootKey = root.id.empty() ? std::string() : stateId(root.id);
        const auto path = "@" + stateId(root.id);
        walkStateNodes(root, counts, path, !root.id.empty() && counts.at(root.id) > 1 ? path : rootKey, visit);
    }
}
#endif
