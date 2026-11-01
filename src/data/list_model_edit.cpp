#include "gran/data/internal/list_notify.hpp"
#include "gran/data/list_model.hpp"

#include <algorithm>
#include <utility>

namespace gran::data
{
    void ListModel::remove(std::size_t index)
    {
        remove(index, 1);
    }

    // Диапазон должен целиком лежать в списке / The range must lie wholly inside the list
    void ListModel::remove(std::size_t index, std::size_t count)
    {
        if (count == 0)
            return;
        internal::checkIndex(index + count - 1, _records.size());
        auto first = _records.begin() + static_cast<std::ptrdiff_t>(index);
        _records.erase(first, first + static_cast<std::ptrdiff_t>(count));
        notifyRemoved(index, count);
    }

    // Равное значение не рассылается: привязке нечего обновлять / An equal value is not delivered: the binding has nothing to update
    void ListModel::set(std::size_t index, const std::string& field, std::string value)
    {
        internal::checkIndex(index, _records.size());
        std::string& slot = _records[index][field];
        if (slot == value)
            return;
        slot = std::move(value);
        notifyChanged(index, 1);
    }

    void ListModel::setRecord(std::size_t index, Record record)
    {
        internal::checkIndex(index, _records.size());
        _records[index] = std::move(record);
        notifyChanged(index, 1);
    }

    /*
    ** std::rotate переставляет блок без копий записей: вперёд — блок уходит за следующие записи,
    ** назад — следующие записи уходят за блок. to — начало блока в новом порядке.
    **
    ** std::rotate moves the block without copying records: forward — the block goes after the following records,
    ** backward — the following records go after the block. to is the block start in the new order.
    */
    void ListModel::move(std::size_t from, std::size_t count, std::size_t to)
    {
        if (count == 0 || from == to)
            return;
        internal::checkIndex(from + count - 1, _records.size());
        internal::checkIndex(to + count - 1, _records.size());
        auto base = _records.begin();
        if (to > from)
            std::rotate(base + static_cast<std::ptrdiff_t>(from), base + static_cast<std::ptrdiff_t>(from + count),
                base + static_cast<std::ptrdiff_t>(to + count));
        else
            std::rotate(base + static_cast<std::ptrdiff_t>(to), base + static_cast<std::ptrdiff_t>(from),
                base + static_cast<std::ptrdiff_t>(from + count));
        notifyMoved(from, count, to);
    }
}
