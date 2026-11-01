#include "gran/data/tree_list.hpp"

#include <utility>

namespace gran::data
{
    void TreeList::setSort(SortKeys keys)
    {
        _keys = std::move(keys);
        refresh();
    }

    const SortKeys& TreeList::sort() const noexcept
    {
        return _keys;
    }

    void TreeList::setFilter(std::shared_ptr<NodeFilter> filter)
    {
        _filter = std::move(filter);
        invalidateFilter();
    }

    /*
    ** Предки подходящих узлов раскрываются один раз — при смене условий фильтра, а не при каждом
    ** пересчёте: пользователь может свернуть ветку, пока фильтр действует.
    **
    ** The ancestors of the matching nodes are expanded once — when the filter conditions change, not at every
    ** recomputation: the user may collapse a branch while the filter is active.
    */
    void TreeList::invalidateFilter()
    {
        build();
        for (const auto& entry : _kept)
        {
            NodeId parent = 0;
            if (entry.second && _tree.contains(entry.first))
                parent = _tree.parent(entry.first);
            if (parent != 0 && parent != _tree.root())
                _expanded.insert(parent);
        }
        refresh();
    }

    void TreeList::refresh()
    {
        publish(build());
    }
}
