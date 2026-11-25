#include "gran/data/sort_filter_proxy.hpp"

#include <algorithm>
#include <utility>

namespace gran::data
{
    void SortFilterProxy::replaceSource(ListSource& base)
    {
        if (_source == &base)
            return;
        SortFilterProxy next(base);
        _source->unsubscribe(this);
        _source = &base;
        const auto removed = _map.size();
        _map.clear();
        _reverseValid = false;
        notifyRemoved(0, removed);
        base.subscribe(this);
        _map.swap(next._map);
        _reverseValid = false;
        notifyInserted(0, _map.size());
    }

    void SortFilterProxy::setSort(SortKeys keys)
    {
        _keys = std::move(keys);
        resort();
    }

    const SortKeys& SortFilterProxy::sort() const noexcept
    {
        return _keys;
    }

    void SortFilterProxy::setFilter(std::shared_ptr<RecordFilter> filter)
    {
        _filter = std::move(filter);
        invalidateFilter();
    }

    /*
    ** Сначала уходят строки, которые фильтр больше не пропускает, затем встают новые — каждая на своё
    ** место: подписчики получают точечные removed/inserted, остальные строки (фокус, выделение) целы.
    **
    ** First the rows the filter no longer passes go away, then the new ones take their places — each in its own
    ** place: the subscribers get point removed/inserted, the other rows (the focus, the selection) stay intact.
    */
    void SortFilterProxy::invalidateFilter()
    {
        std::vector<std::size_t> hidden;
        std::vector<std::size_t> shown;

        for (std::size_t row = _map.size(); row > 0; --row)
        {
            if (!accepts(_map[row - 1]))
                hidden.push_back(row - 1);
        }
        eraseRows(hidden);
        const std::vector<std::size_t>& table = reverse();
        for (std::size_t index = 0; index < _source->size(); ++index)
        {
            if (table[index] == static_cast<std::size_t>(-1) && accepts(index))
                shown.push_back(index);
        }
        for (std::size_t index : shown)
            place(index);
    }

    // Порядок полный (ключи, затем номер источника), поэтому std::sort даёт устойчивый результат / The order is total (the keys, then the source index), so std::sort gives a stable result
    void SortFilterProxy::resort()
    {
        const auto size = _source->size();
        for (std::size_t source : _map)
        {
            if (source >= size)
                return;
        }
        std::vector<std::size_t> sorted = _map;

        std::sort(sorted.begin(), sorted.end(), Order{this});
        if (sorted == _map)
            return;
        std::vector<std::size_t> newRow(size, 0);
        for (std::size_t row = 0; row < sorted.size(); ++row)
            newRow[sorted[row]] = row;
        std::vector<std::size_t> oldToNew(_map.size(), 0);
        for (std::size_t row = 0; row < _map.size(); ++row)
            oldToNew[row] = newRow[_map[row]];
        _map = std::move(sorted);
        _reverseValid = false;
        notifyReordered(oldToNew);
    }
}
