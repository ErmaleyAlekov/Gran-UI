#include "gran/data/tree_list.hpp"

#include <utility>

namespace gran::data
{
    void TreeList::replaceSource(TreeModel& base)
    {
        if (_tree == &base)
            return;
        TreeList next(base);
        _tree->unsubscribe(this);
        _tree = &base;
        const auto removed = _rows.size();
        _rows.clear();
        _expanded.clear();
        _kept.clear();
        notifyRemoved(0, removed);
        notifySourceReplacement();
        base.subscribe(this);
        _rows.swap(next._rows);
        notifyInserted(0, _rows.size());
    }

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
            if (entry.second && _tree->contains(entry.first))
                parent = _tree->parent(entry.first);
            if (parent != 0 && parent != _tree->root())
                _expanded.insert(parent);
        }
        refresh();
    }

    void TreeList::refresh()
    {
        publish(build());
    }
}
