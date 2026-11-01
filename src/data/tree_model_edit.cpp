#include "gran/data/internal/list_notify.hpp"
#include "gran/data/tree_model.hpp"

#include <utility>

namespace gran::data
{
    // Сначала данные, затем уведомление: подписчик видит узел уже на месте / The data first, then the notification: the subscriber sees the node in place already
    NodeId TreeModel::insert(NodeId parentId, std::size_t row, Record record)
    {
        node(parentId);
        std::vector<NodeId>& siblings = _nodes[parentId].children;
        internal::checkIndex(row, siblings.size() + 1);
        NodeId id = _next++;
        Node created;
        created.record = std::move(record);
        created.parent = parentId;
        _nodes.emplace(id, std::move(created));
        _nodes[parentId].children.insert(_nodes[parentId].children.begin() + static_cast<std::ptrdiff_t>(row), id);
        notify(Event::INSERTED, parentId, 0, row, 1);
        return id;
    }

    NodeId TreeModel::append(NodeId parentId, Record record)
    {
        return insert(parentId, childCount(parentId), std::move(record));
    }

    // Корень остаётся всегда: «удалить всё» — это clear / The root always stays: "remove everything" is clear
    void TreeModel::remove(NodeId id)
    {
        NodeId parentId = parent(id);

        if (parentId == 0)
            return;
        std::size_t place = row(id);
        std::vector<NodeId>& siblings = _nodes[parentId].children;
        siblings.erase(siblings.begin() + static_cast<std::ptrdiff_t>(place));
        erase(id);
        notify(Event::REMOVED, parentId, 0, place, 1);
    }

    // Равное значение не рассылается / An equal value is not delivered
    void TreeModel::set(NodeId id, const std::string& field, std::string value)
    {
        node(id);
        std::string& slot = _nodes[id].record[field];
        if (slot == value)
            return;
        slot = std::move(value);
        notify(Event::CHANGED, id, 0, 0, 0);
    }
}
