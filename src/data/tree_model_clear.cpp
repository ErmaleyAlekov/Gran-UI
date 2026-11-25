#include "gran/data/tree_model.hpp"

#include <utility>

namespace gran::data
{
    void TreeModel::clearChildren(NodeId id)
    {
        std::vector<NodeId> children = node(id).children;

        _nodes[id].children.clear();
        for (NodeId child : children)
            erase(child);
        if (!children.empty())
            notify(Event::REMOVED, id, 0, 0, children.size());
    }

    // Обход в глубину без рекурсии: дерево может быть очень глубоким / A depth-first walk without recursion: the tree may be very deep
    void TreeModel::erase(NodeId id)
    {
        std::vector<NodeId> pending{id};

        while (!pending.empty())
        {
            NodeId current = pending.back();
            pending.pop_back();
            auto found = _nodes.find(current);
            if (found == _nodes.end())
                continue;
            pending.insert(pending.end(), found->second.children.begin(), found->second.children.end());
            _nodes.erase(found);
        }
    }

    // Номер первого нового ребёнка запоминается до вставки — его получат подписчики / The row of the first new child is remembered before the insertion — the subscribers get it
    void TreeModel::appendItems(NodeId parentId, std::vector<TreeItem> items)
    {
        std::size_t row = childCount(parentId);
        std::size_t count = items.size();

        for (TreeItem& item : items)
            attach(parentId, std::move(item));
        if (count > 0)
            notify(Event::INSERTED, parentId, 0, row, count);
    }

    void TreeModel::replaceItems(std::vector<TreeItem> items)
    {
        TreeModel staged;
        staged._next = _next;
        staged.appendItems(root(), std::move(items));
        const auto count = staged.childCount(root());
        const auto removed = childCount(root());
        auto header = staged._nodes.extract(root());
        auto& incoming = header.mapped().children;
        TreeModel retired;
        retired._nodes.reserve(staged._nodes.size() + 1);
        std::swap(retired._nodes.at(root()), _nodes.at(root()));
        retired._nodes.at(root()).children.clear();
        _nodes.swap(retired._nodes);
        _next = staged._next;
        if (removed > 0)
            notify(Event::REMOVED, root(), 0, 0, removed);
        auto& children = _nodes.at(root()).children;
        const auto row = children.size();
        if (row > 0)
        {
            children.reserve(row + count);
            _nodes.reserve(_nodes.size() + staged._nodes.size());
        }
        _nodes.merge(staged._nodes);
        if (row == 0)
            children.swap(incoming);
        else
            children.insert(children.end(), incoming.begin(), incoming.end());
        if (count > 0)
            notify(Event::INSERTED, root(), 0, row, count);
    }

    // Стек сохраняет порядок детей, не занимая стек вызовов / The stack preserves child order without growing the call stack
    void TreeModel::attach(NodeId parentId, TreeItem item)
    {
        std::vector<std::pair<NodeId, TreeItem>> pending;
        pending.emplace_back(parentId, std::move(item));

        while (!pending.empty())
        {
            auto [parent, current] = std::move(pending.back());
            pending.pop_back();
            const NodeId id = _next++;
            Node created;
            created.record = std::move(current.record);
            created.parent = parent;
            _nodes.emplace(id, std::move(created));
            _nodes.at(parent).children.push_back(id);
            for (auto child = current.children.rbegin(); child != current.children.rend(); ++child)
                pending.emplace_back(id, std::move(*child));
        }
    }
}
