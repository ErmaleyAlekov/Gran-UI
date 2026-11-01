#include "gran/data/sort_filter_proxy.hpp"

#include <algorithm>

namespace gran::data
{
    bool SortFilterProxy::Order::operator()(std::size_t left, std::size_t right) const
    {
        return proxy->before(left, right);
    }

    bool SortFilterProxy::accepts(std::size_t sourceIndex) const
    {
        return _filter == nullptr || _filter->accepts(_source, sourceIndex);
    }

    // _map всегда упорядочен, поэтому место ищется двоичным поиском / _map is always ordered, so the place is found by a binary search
    std::size_t SortFilterProxy::position(std::size_t sourceIndex) const
    {
        auto found = std::lower_bound(_map.begin(), _map.end(), sourceIndex, Order{this});

        return static_cast<std::size_t>(found - _map.begin());
    }

    // Без ключей порядок — порядок источника / Without keys the order is the source order
    bool SortFilterProxy::before(std::size_t left, std::size_t right) const
    {
        if (!_keys.empty())
        {
            int order = compareRecords(_source.at(left), _source.at(right), _keys);
            if (order != 0)
                return order < 0;
        }
        return left < right;
    }

    void SortFilterProxy::place(std::size_t sourceIndex)
    {
        std::size_t row = position(sourceIndex);

        _map.insert(_map.begin() + static_cast<std::ptrdiff_t>(row), sourceIndex);
        _reverseValid = false;
        notifyInserted(row, 1);
    }
}
