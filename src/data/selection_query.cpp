#include "gran/data/selection_model.hpp"

namespace gran::data
{
    std::size_t SelectionModel::current() const noexcept
    {
        return _current;
    }

    std::size_t SelectionModel::anchor() const noexcept
    {
        return _anchor;
    }

    bool SelectionModel::isSelected(std::size_t index) const noexcept
    {
        return index < _marks.size() && _marks[index] != 0;
    }

    std::size_t SelectionModel::count() const noexcept
    {
        return _count;
    }

    std::vector<std::size_t> SelectionModel::selected() const
    {
        std::vector<std::size_t> result;

        result.reserve(_count);
        for (std::size_t index = 0; index < _marks.size() && result.size() < _count; ++index)
        {
            if (_marks[index] != 0)
                result.push_back(index);
        }
        return result;
    }
}
