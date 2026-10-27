#include "gran/data/internal/list_notify.hpp"

#include <algorithm>
#include <utility>

namespace gran::data
{
    // Равное значение не рассылается: привязке нечего обновлять / An equal value is not delivered: the binding has nothing to update
    void ListModel::set(std::size_t index, const std::string& field, std::string value)
    {
        internal::checkIndex(index, _records.size());
        std::string& slot = _records[index][field];
        if (slot == value)
            return;
        slot = std::move(value);
        internal::deliver(*this, _observers, {internal::ListEventKind::CHANGED, index, 1});
    }

    // Пустая модель не рассылает ничего / An empty model delivers nothing
    void ListModel::clear()
    {
        std::size_t count = _records.size();

        if (count == 0)
            return;
        _records.clear();
        internal::deliver(*this, _observers, {internal::ListEventKind::REMOVED, 0, count});
    }

    void ListModel::subscribe(ListObserver* observer)
    {
        if (observer != nullptr && !subscribed(observer))
            _observers.push_back(observer);
    }

    void ListModel::unsubscribe(ListObserver* observer) noexcept
    {
        _observers.erase(std::remove(_observers.begin(), _observers.end(), observer), _observers.end());
    }

    bool ListModel::subscribed(const ListObserver* observer) const noexcept
    {
        return std::find(_observers.begin(), _observers.end(), observer) != _observers.end();
    }
}
