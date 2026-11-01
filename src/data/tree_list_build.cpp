#include "gran/data/tree_list.hpp"

#include <algorithm>

namespace gran::data
{
    bool TreeList::SiblingOrder::operator()(NodeId left, NodeId right) const
    {
        return compareRecords(tree->record(left), tree->record(right), *keys) < 0;
    }

    // Фильтр считается по всему дереву: свёрнутая ветка тоже может прятать подходящий узел / The filter is computed over the whole tree: a collapsed branch may hide a matching node too
    std::vector<TreeList::Row> TreeList::build()
    {
        std::vector<Row> rows;

        _kept.clear();
        if (_filter != nullptr)
            keep(_tree.root());
        appendChildren(rows, _tree.root(), 0);
        return rows;
    }

    // Признак «есть дети» учитывает фильтр: ветка без подходящих узлов не раскрывается / The "has children" flag respects the filter: a branch without matching nodes does not expand
    void TreeList::appendChildren(std::vector<Row>& rows, NodeId parent, std::uint32_t depth)
    {
        for (NodeId child : visibleChildren(parent))
        {
            Row row;
            row.node = child;
            row.depth = depth;
            row.hasChildren = _tree.lazy(child);
            for (std::size_t index = 0; index < _tree.childCount(child) && !row.hasChildren; ++index)
                row.hasChildren = _filter == nullptr || _kept[_tree.child(child, index)];
            row.expanded = row.hasChildren && _expanded.count(child) > 0;
            rows.push_back(row);
            if (row.expanded)
                appendChildren(rows, child, depth + 1);
        }
    }

    // Устойчивая сортировка: равные по ключам братья остаются в порядке дерева / A stable sort: siblings equal by the keys stay in the tree order
    std::vector<NodeId> TreeList::visibleChildren(NodeId parent) const
    {
        std::vector<NodeId> children;

        for (std::size_t index = 0; index < _tree.childCount(parent); ++index)
        {
            NodeId child = _tree.child(parent, index);
            auto kept = _kept.find(child);
            if (_filter == nullptr || (kept != _kept.end() && kept->second))
                children.push_back(child);
        }
        if (!_keys.empty())
            std::stable_sort(children.begin(), children.end(), SiblingOrder{&_tree, &_keys});
        return children;
    }

    // Обход после детей: узел остаётся, если подходит сам или остался хоть один потомок / A post-order walk: a node stays if it matches by itself or at least one descendant stayed
    bool TreeList::keep(NodeId node)
    {
        bool result = node != _tree.root() && _filter->accepts(_tree, node);

        for (std::size_t index = 0; index < _tree.childCount(node); ++index)
            result = keep(_tree.child(node, index)) || result;
        _kept[node] = result;
        return result;
    }
}
