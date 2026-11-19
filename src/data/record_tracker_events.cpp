#include "gran/data/record_tracker.hpp"

namespace gran::data
{
    // Вставка перед записью или на её место сдвигает её вперёд / An insertion before the record or at its place shifts it forward
    void RecordTracker::inserted(std::size_t index, std::size_t count)
    {
        if (_alive && index <= _index)
            _index += count;
    }

    // Запись внутри удалённого участка умирает, после него — сдвигается назад / A record inside the removed range dies, after it — shifts back
    void RecordTracker::removed(std::size_t index, std::size_t count)
    {
        if (!_alive || _index < index)
            return;
        if (_index - index < count)
            _alive = false;
        else
            _index -= count;
    }

    // Поля записи сменились — номер тот же / The record fields changed — the index is the same
    void RecordTracker::changed(std::size_t)
    {
    }

    /*
    ** Запись внутри переставленного участка встаёт на to + смещение в участке. Иначе — как удаление участка
    ** и вставка его на место to (to — номер в новом порядке): сначала сдвиг назад, если участок был до записи,
    ** затем вперёд, если участок встал до неё.
    **
    ** A record inside the moved range goes to to + its offset in the range. Otherwise — as removing the range
    ** and inserting it at to (to is an index in the new order): first a shift back if the range was before the record,
    ** then forward if the range went before it.
    */
    void RecordTracker::moved(std::size_t from, std::size_t count, std::size_t to)
    {
        if (!_alive)
            return;
        if (_index >= from && _index - from < count)
        {
            _index = to + (_index - from);
            return;
        }
        if (_index >= from + count)
            _index -= count;
        if (_index >= to)
            _index += count;
    }

    // Перестановка того же набора записей: новый номер — по таблице (вне её — запись потеряна) / A permutation of the same record set: the new index is by the table (outside it — the record is lost)
    void RecordTracker::reordered(const std::vector<std::size_t>& oldToNew)
    {
        if (!_alive)
            return;
        if (_index < oldToNew.size())
            _index = oldToNew[_index];
        else
            _alive = false;
    }
}
