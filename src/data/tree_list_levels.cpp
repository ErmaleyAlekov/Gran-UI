#include "gran/data/tree_list.hpp"

namespace gran::data
{
    namespace
    {
        // Узлы с глубиной меньше levels попадают в набор раскрытых / The nodes with a depth less than levels go into the expanded set
        void collectLevels(const TreeModel& tree, NodeId parent, std::size_t levels, std::unordered_set<NodeId>& result)
        {
            if (levels == 0)
                return;
            for (std::size_t row = 0; row < tree.childCount(parent); ++row)
            {
                NodeId child = tree.child(parent, row);
                if (!tree.hasChildren(child))
                    continue;
                result.insert(child);
                collectLevels(tree, child, levels - 1, result);
            }
        }
    }

    // Ленивые узлы первых уровней подгружают детей сразу / The lazy nodes of the first levels fetch their children at once
    void TreeList::expandToDepth(std::size_t levels)
    {
        std::unordered_set<NodeId> expanded;

        collectLevels(_tree, _tree.root(), levels, expanded);
        _expanded = expanded;
        for (NodeId node : expanded)
        {
            if (_tree.contains(node) && _tree.lazy(node))
                _tree.fetchChildren(node);
        }
        refresh();
    }
}
