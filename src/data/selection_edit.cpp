#include "gran/data/internal/list_notify.hpp"
#include "gran/data/selection_model.hpp"

#include <algorithm>

namespace gran::data
{
    // В режиме NONE отметок нет; SINGLE перед отметкой снимает прочие / The NONE mode has no marks; SINGLE drops the others before marking
    void SelectionModel::select(std::size_t index)
    {
        internal::checkIndex(index, _marks.size());
        if (_mode == SelectionMode::NONE || isSelected(index))
            return;
        if (_mode == SelectionMode::SINGLE)
        {
            unmarkAll();
        }
        mark(index, true);
        publish();
    }

    void SelectionModel::deselect(std::size_t index)
    {
        internal::checkIndex(index, _marks.size());
        if (mark(index, false))
            publish();
    }

    void SelectionModel::toggle(std::size_t index)
    {
        if (isSelected(index))
            deselect(index);
        else
            select(index);
    }

    // В режиме SINGLE диапазон сводится к последней записи / In the SINGLE mode a range comes down to the last record
    void SelectionModel::selectRange(std::size_t first, std::size_t last)
    {
        internal::checkIndex(first, _marks.size());
        internal::checkIndex(last, _marks.size());
        if (_mode == SelectionMode::NONE)
            return;
        if (_mode == SelectionMode::SINGLE)
        {
            select(last);
            return;
        }
        bool changed = false;
        for (std::size_t index = std::min(first, last); index <= std::max(first, last); ++index)
            changed = mark(index, true) || changed;
        if (changed)
            publish();
    }

    void SelectionModel::clear()
    {
        // Скрытые отметки (свёрнутая ветка) тоже снимаются / The hidden marks (a collapsed branch) are dropped too
        if (_count == 0 && _hidden.empty())
            return;
        unmarkAll();
        publish();
    }
}
