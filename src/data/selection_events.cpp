#include "gran/data/internal/list_notify.hpp"
#include "gran/data/selection_model.hpp"

namespace gran::data
{
    namespace
    {
        // Номер после удаления [index, index + count): удалённый — NO_ROW / An index after removing [index, index + count): a removed one is NO_ROW
        std::size_t afterRemoval(std::size_t row, std::size_t index, std::size_t count) noexcept
        {
            if (row == NO_ROW || row < index)
                return row;
            if (row < index + count)
                return NO_ROW;
            return row - count;
        }
    }

    // Новые записи не отмечены; текущая и опорная сдвигаются вместе со своими записями / New records are not marked; the current and the anchor shift with their records
    void SelectionModel::inserted(std::size_t index, std::size_t count)
    {
        _marks.insert(_marks.begin() + static_cast<std::ptrdiff_t>(index), count, 0);
        restoreInserted(index, count);
        if (_current != NO_ROW && _current >= index)
            _current += count;
        if (_anchor != NO_ROW && _anchor >= index)
            _anchor += count;
        if (_count > 0 || _current != NO_ROW)
            publish();
    }

    // Удалённая текущая запись передаёт роль соседу (следующей, у конца — предыдущей) / A removed current record passes the role to a neighbour (the next one, at the end — the previous one)
    void SelectionModel::removed(std::size_t index, std::size_t count)
    {
        bool lostCurrent = _current != NO_ROW && afterRemoval(_current, index, count) == NO_ROW;

        hideRemoved(index, count);
        for (std::size_t row = index; row < index + count && row < _marks.size(); ++row)
            mark(row, false);
        auto first = _marks.begin() + static_cast<std::ptrdiff_t>(index);
        _marks.erase(first, first + static_cast<std::ptrdiff_t>(count));
        _current = afterRemoval(_current, index, count);
        _anchor = afterRemoval(_anchor, index, count);
        if (lostCurrent && !_marks.empty())
        {
            _current = index;
            // В дереве перед удалённым блоком стоит его родитель: свёрнутая ветка отдаёт текущую ему, как Проводник
            // In a tree the removed block is preceded by its parent: a collapsed branch gives the current role to it, as Explorer does
            if (_source.hasRowKeys() && index > 0)
                _current = index - 1;
            if (_current >= _marks.size())
                _current = _marks.size() - 1;
        }
        if (_anchor == NO_ROW)
            _anchor = _current;
        publish();
    }

    void SelectionModel::moved(std::size_t from, std::size_t count, std::size_t to)
    {
        reordered(internal::moveMapping(_marks.size(), from, count, to));
    }

    void SelectionModel::reordered(const std::vector<std::size_t>& oldToNew)
    {
        std::vector<std::uint8_t> marks(_marks.size(), 0);

        for (std::size_t row = 0; row < oldToNew.size() && row < _marks.size(); ++row)
            marks[oldToNew[row]] = _marks[row];
        _marks = std::move(marks);
        if (_current != NO_ROW && _current < oldToNew.size())
            _current = oldToNew[_current];
        if (_anchor != NO_ROW && _anchor < oldToNew.size())
            _anchor = oldToNew[_anchor];
        readKeys();
        publish();
    }
}
