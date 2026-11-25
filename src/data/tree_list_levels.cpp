#include "gran/data/tree_list.hpp"

namespace gran::data
{
    namespace
    {
        // Узлы с глубиной меньше levels попадают в набор раскрытых / The nodes with a depth less than levels go into the expanded set
        void collectLevels(const TreeModel& tree, NodeId parent, std::size_t levels, std::unordered_set<NodeId>& result)
        {
            std::vector<std::pair<NodeId, std::size_t>> pending{{parent, levels}};
            while (!pending.empty())
            {
                const auto [current, remaining] = pending.back();
                pending.pop_back();
                if (current != parent)
                    result.insert(current);
                if (remaining == 0)
                    continue;
                for (std::size_t row = tree.childCount(current); row > 0; --row)
                {
                    const auto child = tree.child(current, row - 1);
                    if (!tree.hasChildren(child))
                        continue;
                    pending.emplace_back(child, remaining - 1);
                }
            }
        }
    }

    // Ленивые узлы первых уровней подгружают детей сразу / The lazy nodes of the first levels fetch their children at once
    void TreeList::expandToDepth(std::size_t levels)
    {
        std::unordered_set<NodeId> expanded;

        collectLevels(*_tree, _tree->root(), levels, expanded);
        _expanded = expanded;
        for (NodeId node : expanded)
        {
            if (_tree->contains(node) && _tree->lazy(node))
                _tree->fetchChildren(node);
        }
        refresh();
    }

    const std::shared_ptr<NodeFilter>& TreeList::filter() const noexcept
    {
        return _filter;
    }
}
