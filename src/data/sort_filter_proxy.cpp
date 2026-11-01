#include "gran/data/internal/list_notify.hpp"
#include "gran/data/sort_filter_proxy.hpp"

#include <utility>

namespace gran::data
{
    // Сначала все записи источника по порядку, потом подписка: уведомлять пока некого / All source records in order first, then the subscription: nobody to notify yet
    SortFilterProxy::SortFilterProxy(ListSource& source)
        : _source(source)
    {
        _map.reserve(source.size());
        for (std::size_t index = 0; index < source.size(); ++index)
            _map.push_back(index);
        _source.subscribe(this);
    }

    SortFilterProxy::~SortFilterProxy()
    {
        _source.unsubscribe(this);
    }

    std::size_t SortFilterProxy::size() const noexcept
    {
        return _map.size();
    }

    const Record& SortFilterProxy::at(std::size_t index) const
    {
        internal::checkIndex(index, _map.size());
        return _source.at(_map[index]);
    }

    // Источник сам разошлёт changed, а прокси переставит строку, если ключ изменился / The source delivers changed itself, and the proxy moves the row if the key changed
    void SortFilterProxy::set(std::size_t index, const std::string& field, std::string value)
    {
        internal::checkIndex(index, _map.size());
        _source.set(_map[index], field, std::move(value));
    }
}
