#include "gran/data/list_source.hpp"

#include <algorithm>

namespace gran::data
{
    // Хранилище без подгрузчика ничего не подгружает / A storage without a fetcher fetches nothing
    bool ListSource::canFetchMore() const
    {
        return false;
    }

    void ListSource::fetchMore()
    {
    }

    void ListSource::subscribe(ListObserver* observer)
    {
        if (observer != nullptr && !subscribed(observer))
            _observers.push_back(observer);
    }

    void ListSource::unsubscribe(ListObserver* observer) noexcept
    {
        _observers.erase(std::remove(_observers.begin(), _observers.end(), observer), _observers.end());
    }

    bool ListSource::subscribed(const ListObserver* observer) const noexcept
    {
        return std::find(_observers.begin(), _observers.end(), observer) != _observers.end();
    }
}
