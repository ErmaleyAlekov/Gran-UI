#include "gran/data/selection_model.hpp"

#include <algorithm>

namespace gran::data
{
    SelectionModel::SelectionModel(ListSource& source, SelectionMode mode)
        : _source(source), _mode(mode), _marks(source.size(), 0)
    {
        _source.subscribe(this);
    }

    SelectionModel::~SelectionModel()
    {
        _source.unsubscribe(this);
    }

    ListSource& SelectionModel::source() const noexcept
    {
        return _source;
    }

    SelectionMode SelectionModel::mode() const noexcept
    {
        return _mode;
    }

    /*
    ** NONE снимает все отметки, SINGLE оставляет одну — текущую, если она отмечена, иначе первую.
    **
    ** NONE drops all marks, SINGLE keeps one — the current one if it is marked, otherwise the first one.
    */
    void SelectionModel::setMode(SelectionMode mode)
    {
        std::size_t keep = NO_ROW;

        _mode = mode;
        if (_count == 0 || mode == SelectionMode::MULTI || mode == SelectionMode::EXTENDED)
            return;
        if (mode == SelectionMode::SINGLE && _current != NO_ROW && isSelected(_current))
            keep = _current;
        else if (mode == SelectionMode::SINGLE)
            keep = static_cast<std::size_t>(std::find(_marks.begin(), _marks.end(), 1) - _marks.begin());
        std::fill(_marks.begin(), _marks.end(), 0);
        _count = 0;
        if (keep != NO_ROW)
            mark(keep, true);
        publish();
    }
}
