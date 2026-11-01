#include "gran/data/tree_model.hpp"

#include <algorithm>

namespace gran::data
{
    NodeId TreeModel::parent(NodeId id) const
    {
        return node(id).parent;
    }

    // Детей у узла обычно немного — линейный поиск дешевле обратного словаря / A node usually has few children — a linear search is cheaper than a reverse map
    std::size_t TreeModel::row(NodeId id) const
    {
        const Node& item = node(id);

        if (item.parent == 0)
            return 0;
        const std::vector<NodeId>& siblings = node(item.parent).children;
        return static_cast<std::size_t>(std::find(siblings.begin(), siblings.end(), id) - siblings.begin());
    }

    const Record& TreeModel::record(NodeId id) const
    {
        return node(id).record;
    }

    bool TreeModel::lazy(NodeId id) const
    {
        return node(id).lazy;
    }

    bool TreeModel::hasChildren(NodeId id) const
    {
        const Node& item = node(id);

        return item.lazy || !item.children.empty();
    }
}
