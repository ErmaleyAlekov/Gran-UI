#include "gran/data/sort_filter_proxy.hpp"

namespace gran::data
{
    bool SortFilterProxy::canFetchMore() const
    {
        return _source.canFetchMore();
    }

    // Новые записи придут от источника обычными inserted / The new records come from the source as ordinary inserted
    void SortFilterProxy::fetchMore()
    {
        _source.fetchMore();
    }

    /*
    ** Строки идут по убыванию; соседние (5, 4, 3) уходят одним уведомлением removed(3, 3).
    ** Каждое уведомление — после изъятия своего участка: подписчик видит согласованное состояние.
    **
    ** The rows go descending; neighbouring ones (5, 4, 3) go with one notification removed(3, 3).
    ** Each notification comes after taking out its run: the subscriber sees a consistent state.
    */
    void SortFilterProxy::eraseRows(const std::vector<std::size_t>& rows)
    {
        std::size_t index = 0;

        while (index < rows.size())
        {
            std::size_t last = rows[index];
            std::size_t first = last;
            while (index + 1 < rows.size() && rows[index + 1] + 1 == first)
                first = rows[++index];
            ++index;
            _map.erase(_map.begin() + static_cast<std::ptrdiff_t>(first),
                _map.begin() + static_cast<std::ptrdiff_t>(last + 1));
            _reverseValid = false;
            notifyRemoved(first, last - first + 1);
        }
    }
}
