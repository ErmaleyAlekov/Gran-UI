#ifndef GRAN_DATA_RECORD_TRACKER_HPP
#define GRAN_DATA_RECORD_TRACKER_HPP

#include "gran/data/list_source.hpp"

#include <cstddef>
#include <vector>

namespace gran::data
{
    /*
    ** Следит за одной записью источника: её номер меняется при вставках, удалениях, перемещениях
    ** и пересортировке, удалённая запись «умирает». Нужен тому, кто держит запись дольше одного вызова:
    ** правке ячейки таблицы (пока открыт редактор, записи могут добавить, удалить или отсортировать).
    ** Подписывается в конструкторе и отписывается в деструкторе — источник должен жить дольше.
    **
    ** Watches one source record: its index changes on insertions, removals, moves
    ** and re-sorting, a removed record "dies". It is needed by whoever keeps a record longer than one call:
    ** a table cell edit (while the editor is open the records may be added, removed or sorted).
    ** It subscribes in the constructor and unsubscribes in the destructor — the source must outlive it.
    */
    class RecordTracker final : public ListObserver
    {
    public:
        // ---- record_tracker.cpp ----

        // index вне источника — запись сразу считается удалённой / index outside the source — the record counts as removed at once
        RecordTracker(ListSource& source, std::size_t index);
        ~RecordTracker() override;
        RecordTracker(const RecordTracker&) = delete;
        RecordTracker& operator=(const RecordTracker&) = delete;

        // Источник записи / The record source
        ListSource& source() const noexcept;

        // Нынешний номер записи (имеет смысл, только если alive) / The current record index (meaningful only if alive)
        std::size_t index() const noexcept;

        // Запись ещё есть в источнике / The record is still in the source
        bool alive() const noexcept;

        // ---- record_tracker_events.cpp ----

        void inserted(std::size_t index, std::size_t count) override;
        void removed(std::size_t index, std::size_t count) override;
        void changed(std::size_t index) override;
        void moved(std::size_t from, std::size_t count, std::size_t to) override;
        void reordered(const std::vector<std::size_t>& oldToNew) override;

    private:
        ListSource& _source;
        std::size_t _index;
        bool _alive;
    };
}

#endif
