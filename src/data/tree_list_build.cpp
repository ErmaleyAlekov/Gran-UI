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
            keep(_tree->root());
        appendChildren(rows, _tree->root(), 0);
        return rows;
    }

    // Признак «есть дети» учитывает фильтр: ветка без подходящих узлов не раскрывается / The "has children" flag respects the filter: a branch without matching nodes does not expand
    void TreeList::appendChildren(std::vector<Row>& rows, NodeId parent, std::uint32_t depth)
    {
        std::vector<std::pair<NodeId, std::uint32_t>> pending;
        auto children = visibleChildren(parent);
        for (auto child = children.rbegin(); child != children.rend(); ++child)
            pending.emplace_back(*child, depth);

        while (!pending.empty())
        {
            const auto [child, level] = pending.back();
            pending.pop_back();
            Row row;
            row.node = child;
            row.depth = level;
            row.hasChildren = _tree->lazy(child);
            for (std::size_t index = 0; index < _tree->childCount(child) && !row.hasChildren; ++index)
                row.hasChildren = _filter == nullptr || _kept[_tree->child(child, index)];
            row.expanded = row.hasChildren && _expanded.count(child) > 0;
            rows.push_back(row);
            if (!row.expanded)
                continue;
            children = visibleChildren(child);
            for (auto descendant = children.rbegin(); descendant != children.rend(); ++descendant)
                pending.emplace_back(*descendant, level + 1);
        }
    }

    // Устойчивая сортировка: равные по ключам братья остаются в порядке дерева / A stable sort: siblings equal by the keys stay in the tree order
    std::vector<NodeId> TreeList::visibleChildren(NodeId parent) const
    {
        std::vector<NodeId> children;

        for (std::size_t index = 0; index < _tree->childCount(parent); ++index)
        {
            NodeId child = _tree->child(parent, index);
            auto kept = _kept.find(child);
            if (_filter == nullptr || (kept != _kept.end() && kept->second))
                children.push_back(child);
        }
        if (!_keys.empty())
            std::stable_sort(children.begin(), children.end(), SiblingOrder{_tree, &_keys});
        return children;
    }

    // Обход после детей: узел остаётся, если подходит сам или остался хоть один потомок / A post-order walk: a node stays if it matches by itself or at least one descendant stayed
    bool TreeList::keep(NodeId node)
    {
        std::vector<std::pair<NodeId, std::size_t>> pending{{node, 0}};
        _kept[node] = node != _tree->root() && _filter->accepts(*_tree, node);
        while (!pending.empty())
        {
            auto& [current, next] = pending.back();
            if (next < _tree->childCount(current))
            {
                const auto child = _tree->child(current, next++);
                _kept[child] = _filter->accepts(*_tree, child);
                pending.emplace_back(child, 0);
                continue;
            }
            const bool kept = _kept.at(current);
            pending.pop_back();
            if (kept && !pending.empty())
                _kept.at(pending.back().first) = true;
        }
        return _kept.at(node);
    }
}
