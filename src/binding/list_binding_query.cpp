#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    const ui::Node& ListBinding::container() const noexcept
    {
        return _container;
    }

    data::ListSource& ListBinding::model() const noexcept
    {
        return _model;
    }

    data::SelectionModel* ListBinding::selection() const noexcept
    {
        return _selection;
    }

    // Записей на экране немного, линейного поиска достаточно (виртуализация — отдельная задача)
    // There are few records on screen, a linear search is enough (virtualization is a separate task)
    std::size_t ListBinding::indexOf(const ui::Node& item) const noexcept
    {
        for (std::size_t index = 0; index < _items.size(); ++index)
        {
            if (_items[index].root == &item)
                return _first + index;
        }
        return static_cast<std::size_t>(-1);
    }

    ui::Node* ListBinding::itemAt(std::size_t index) const noexcept
    {
        if (index < _first || index - _first >= _items.size())
            return nullptr;
        return _items[index - _first].root;
    }
}
