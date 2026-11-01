#include "gran/data/internal/list_notify.hpp"
#include "gran/data/sort_filter_proxy.hpp"

namespace gran::data
{
    ListSource& SortFilterProxy::source() const noexcept
    {
        return _source;
    }

    std::size_t SortFilterProxy::sourceIndex(std::size_t index) const
    {
        internal::checkIndex(index, _map.size());
        return _map[index];
    }

    std::size_t SortFilterProxy::proxyIndex(std::size_t sourceIndex) const noexcept
    {
        const std::vector<std::size_t>& table = reverse();

        if (sourceIndex >= table.size())
            return static_cast<std::size_t>(-1);
        return table[sourceIndex];
    }

    const ColumnSet& SortFilterProxy::columns() const noexcept
    {
        return _source.columns();
    }

    // Таблица нужна редко (выделение, события), а правки идут пачками — пересчёт по запросу / The table is needed rarely (selection, events), while edits come in batches — recomputing on demand
    const std::vector<std::size_t>& SortFilterProxy::reverse() const
    {
        if (_reverseValid)
            return _reverse;
        _reverse.assign(_source.size(), static_cast<std::size_t>(-1));
        for (std::size_t row = 0; row < _map.size(); ++row)
        {
            if (_map[row] < _reverse.size())
                _reverse[_map[row]] = row;
        }
        _reverseValid = true;
        return _reverse;
    }
}
