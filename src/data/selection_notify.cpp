#include "gran/data/selection_model.hpp"

#include <algorithm>

namespace gran::data
{
    void SelectionModel::subscribe(SelectionObserver* observer)
    {
        if (observer != nullptr && std::find(_observers.begin(), _observers.end(), observer) == _observers.end())
            _observers.push_back(observer);
    }

    void SelectionModel::unsubscribe(SelectionObserver* observer) noexcept
    {
        _observers.erase(std::remove(_observers.begin(), _observers.end(), observer), _observers.end());
    }

    bool SelectionModel::mark(std::size_t index, bool value)
    {
        if ((_marks[index] != 0) == value)
            return false;
        _marks[index] = static_cast<std::uint8_t>(value);
        if (value)
            ++_count;
        else
            --_count;
        return true;
    }

    // Копия списка: подписчик может отписаться во время рассылки / A list copy: a subscriber may unsubscribe during the delivery
    void SelectionModel::publish()
    {
        std::vector<SelectionObserver*> observers = _observers;

        ++_version;
        for (SelectionObserver* observer : observers)
        {
            if (std::find(_observers.begin(), _observers.end(), observer) != _observers.end())
                observer->selectionChanged();
        }
    }

    // Зеркало выделения держит копии отмеченных записей: правка такой записи — тоже изменение / The selection mirror keeps copies of the marked records: editing such a record is a change too
    void SelectionModel::changed(std::size_t index)
    {
        if (isSelected(index) || index == _current)
            publish();
    }
}
