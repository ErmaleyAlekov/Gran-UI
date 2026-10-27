#include "gran/data/internal/list_notify.hpp"

#include <utility>

namespace gran::data
{
    std::size_t ListModel::size() const noexcept
    {
        return _records.size();
    }

    const Record& ListModel::at(std::size_t index) const
    {
        internal::checkIndex(index, _records.size());
        return _records[index];
    }

    // Сначала данные, затем уведомление: подписчик читает уже вставленную запись
    // The data first, then the notification: the subscriber reads the record already inserted
    void ListModel::insert(std::size_t index, Record record)
    {
        internal::checkIndex(index, _records.size() + 1);
        _records.insert(_records.begin() + static_cast<std::ptrdiff_t>(index), std::move(record));
        internal::deliver(*this, _observers, {internal::ListEventKind::INSERTED, index, 1});
    }

    void ListModel::append(Record record)
    {
        insert(_records.size(), std::move(record));
    }

    void ListModel::remove(std::size_t index)
    {
        internal::checkIndex(index, _records.size());
        _records.erase(_records.begin() + static_cast<std::ptrdiff_t>(index));
        internal::deliver(*this, _observers, {internal::ListEventKind::REMOVED, index, 1});
    }
}
