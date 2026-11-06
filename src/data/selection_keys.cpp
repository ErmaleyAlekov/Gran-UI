#include "gran/data/selection_model.hpp"

#include <algorithm>

namespace gran::data
{
    /*
    ** Отметки по ключам записей (паттерн «Хранитель»: модель запоминает отметки исчезнувших строк, чтобы
    ** вернуть их, когда строки появятся снова). Нужны дереву: свёртка ветки — это удаление строк её потомков,
    ** раскрытие — их вставка, и по одним номерам выделение потомков терялось (найдено в ручной проверке Т-2,
    ** 2026-11-06). У источника без ключей все функции ниже ничего не делают — поведение прежнее.
    **
    ** Marks by record keys (the Memento pattern: the model remembers the marks of the vanished rows to give them
    ** back when the rows appear again). The tree needs it: a branch collapse is a removal of its descendant rows,
    ** an expansion is their insertion, and by indices alone the descendant selection was lost (found in the manual
    ** check T-2, 2026-11-06). For a source without keys all functions below do nothing — the behaviour is as before.
    */
    void SelectionModel::unmarkAll() noexcept
    {
        std::fill(_marks.begin(), _marks.end(), 0);
        _count = 0;
        _hidden.clear();
    }

    void SelectionModel::readKeys()
    {
        if (!_source.hasRowKeys())
            return;
        _keys.resize(_source.size());
        for (std::size_t row = 0; row < _keys.size(); ++row)
            _keys[row] = _source.rowKey(row);
    }

    // Удалённые строки источник уже не знает — ключи берутся из своей копии / The source no longer knows the removed rows — the keys come from the own copy
    void SelectionModel::hideRemoved(std::size_t index, std::size_t count)
    {
        std::size_t end = std::min(index + count, _keys.size());

        if (index >= end)
            return;
        for (std::size_t row = index; row < end; ++row)
        {
            if (row < _marks.size() && _marks[row] != 0)
                _hidden.insert(_keys[row]);
        }
        _keys.erase(_keys.begin() + static_cast<std::ptrdiff_t>(index), _keys.begin() + static_cast<std::ptrdiff_t>(end));
    }

    // Строки уже вставлены и в источник, и в _marks: ключи читаются у источника / The rows are already inserted both into the source and into _marks: the keys are read from the source
    void SelectionModel::restoreInserted(std::size_t index, std::size_t count)
    {
        std::vector<std::uint64_t> added;

        if (!_source.hasRowKeys() || index > _keys.size())
            return;
        added.reserve(count);
        for (std::size_t row = index; row < index + count && row < _source.size(); ++row)
            added.push_back(_source.rowKey(row));
        _keys.insert(_keys.begin() + static_cast<std::ptrdiff_t>(index), added.begin(), added.end());
        for (std::size_t offset = 0; offset < added.size() && !_hidden.empty(); ++offset)
        {
            // Ключ возвращается один раз: запись снова видна, дальше её отметка — обычная
            // A key comes back once: the record is visible again, its mark is an ordinary one from now on
            if (_hidden.erase(added[offset]) > 0 && index + offset < _marks.size())
                mark(index + offset, true);
        }
    }
}
