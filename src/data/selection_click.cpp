#include "gran/data/internal/list_notify.hpp"
#include "gran/data/selection_model.hpp"

#include <algorithm>

namespace gran::data
{
    void SelectionModel::selectAll()
    {
        if (_mode != SelectionMode::MULTI && _mode != SelectionMode::EXTENDED)
            return;
        if (_count == _marks.size())
            return;
        std::fill(_marks.begin(), _marks.end(), 1);
        _count = _marks.size();
        publish();
    }

    void SelectionModel::setCurrent(std::size_t index)
    {
        if (index != NO_ROW)
            internal::checkIndex(index, _marks.size());
        if (index == _current)
            return;
        _current = index;
        _anchor = index;
        publish();
    }

    /*
    ** Правила проводника: без модификаторов — только эта запись; Ctrl (EXTENDED) и любой щелчок (MULTI)
    ** сохраняют прежние отметки и переключают эту; Shift отмечает диапазон от опорной записи, не сдвигая её.
    **
    ** The file manager rules: without modifiers — only this record; Ctrl (EXTENDED) and any click (MULTI)
    ** keep the former marks and toggle this one; Shift marks a range from the anchor record without moving it.
    */
    void SelectionModel::activate(std::size_t index, bool control, bool shift)
    {
        internal::checkIndex(index, _marks.size());
        bool many = _mode == SelectionMode::MULTI || _mode == SelectionMode::EXTENDED;
        bool range = shift && many && _anchor != NO_ROW && _anchor < _marks.size();
        bool keep = _mode == SelectionMode::MULTI || (control && _mode == SelectionMode::EXTENDED);

        if (!keep && _mode != SelectionMode::NONE)
        {
            unmarkAll();
        }
        if (range)
        {
            for (std::size_t row = std::min(index, _anchor); row <= std::max(index, _anchor); ++row)
                mark(row, true);
        }
        else if (keep)
            mark(index, !isSelected(index));
        else if (_mode != SelectionMode::NONE)
            mark(index, true);
        _current = index;
        if (!range)
            _anchor = index;
        publish();
    }

    std::uint64_t SelectionModel::version() const noexcept
    {
        return _version;
    }
}
