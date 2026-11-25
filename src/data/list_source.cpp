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
            _observers.push_back(std::make_shared<ListObserver* const>(observer));
    }

    void ListSource::unsubscribe(ListObserver* observer) noexcept
    {
        _observers.erase(std::remove_if(_observers.begin(), _observers.end(),
            [observer](const auto& subscription) { return *subscription == observer; }), _observers.end());
    }

    bool ListSource::subscribed(const ListObserver* observer) const noexcept
    {
        return std::any_of(_observers.begin(), _observers.end(),
            [observer](const auto& subscription) { return *subscription == observer; });
    }
}
