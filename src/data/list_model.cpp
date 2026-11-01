#include "gran/data/internal/list_notify.hpp"
#include "gran/data/list_model.hpp"

#include <iterator>
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
        notifyInserted(index, 1);
    }

    void ListModel::insertAll(std::size_t index, std::vector<Record> records)
    {
        std::size_t count = records.size();

        internal::checkIndex(index, _records.size() + 1);
        _records.insert(_records.begin() + static_cast<std::ptrdiff_t>(index),
            std::make_move_iterator(records.begin()), std::make_move_iterator(records.end()));
        notifyInserted(index, count);
    }

    void ListModel::append(Record record)
    {
        insert(_records.size(), std::move(record));
    }
}
