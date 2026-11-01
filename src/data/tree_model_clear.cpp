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

    // Рекурсия по глубине дерева источника; ширина обходится циклом / A recursion by the source tree depth; the width is walked by a loop
    void TreeModel::attach(NodeId parentId, TreeItem item)
    {
        NodeId id = _next++;
        Node created;

        created.record = std::move(item.record);
        created.parent = parentId;
        _nodes.emplace(id, std::move(created));
        _nodes[parentId].children.push_back(id);
        for (TreeItem& child : item.children)
            attach(id, std::move(child));
    }
}
