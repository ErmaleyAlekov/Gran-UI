#include "gran/data/internal/list_notify.hpp"
#include "gran/data/tree_model.hpp"

namespace gran::data
{
    namespace
    {
        // Номер невидимого корня; 0 — «нет узла» / The invisible root number; 0 — "no node"
        constexpr NodeId ROOT = 1;
    }

    TreeModel::TreeModel()
    {
        _nodes.emplace(ROOT, Node());
    }

    NodeId TreeModel::root() const noexcept
    {
        return ROOT;
    }

    bool TreeModel::contains(NodeId id) const noexcept
    {
        return _nodes.find(id) != _nodes.end();
    }

    std::size_t TreeModel::childCount(NodeId id) const
    {
        return node(id).children.size();
    }

    // Номер ребёнка проверяется так же, как номер записи списка / The child row is checked the same way as a list record index
    NodeId TreeModel::child(NodeId id, std::size_t row) const
    {
        const Node& parent = node(id);

        internal::checkIndex(row, parent.children.size());
        return parent.children[row];
    }
}
