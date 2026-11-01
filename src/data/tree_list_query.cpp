#include "gran/data/internal/list_notify.hpp"
#include "gran/data/tree_list.hpp"

namespace gran::data
{
    TreeModel& TreeList::tree() const noexcept
    {
        return _tree;
    }

    NodeId TreeList::node(std::size_t row) const
    {
        internal::checkIndex(row, _rows.size());
        return _rows[row].node;
    }

    std::size_t TreeList::depth(std::size_t row) const
    {
        internal::checkIndex(row, _rows.size());
        return _rows[row].depth;
    }

    bool TreeList::expanded(std::size_t row) const
    {
        internal::checkIndex(row, _rows.size());
        return _rows[row].expanded;
    }

    bool TreeList::hasChildren(std::size_t row) const
    {
        internal::checkIndex(row, _rows.size());
        return _rows[row].hasChildren;
    }
}
