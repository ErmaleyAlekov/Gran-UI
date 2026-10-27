#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    const ui::Node& ListBinding::container() const noexcept
    {
        return _container;
    }

    data::ListModel& ListBinding::model() const noexcept
    {
        return _model;
    }

    // Записей на экране немного, линейного поиска достаточно (виртуализация — отдельная задача)
    // There are few records on screen, a linear search is enough (virtualization is a separate task)
    std::size_t ListBinding::indexOf(const ui::Node& item) const noexcept
    {
        for (std::size_t index = 0; index < _items.size(); ++index)
        {
            if (_items[index].root == &item)
                return index;
        }
        return static_cast<std::size_t>(-1);
    }

    /*
    ** Копия записи index встаёт сразу за копией записи index − 1, первая — на место прежней
    ** первой; пока копий нет — после всех детей контейнера.
    **
    ** The copy of the record index goes right after the copy of the record index − 1, the first one — in place of
    ** the former first one; while there are no copies — after all container children.
    */
    std::size_t ListBinding::childPosition(std::size_t index) const noexcept
    {
        const ui::Node* neighbour = nullptr;
        std::size_t shift = 0;

        if (index > 0)
        {
            neighbour = _items[index - 1].root;
            shift = 1;
        }
        else if (!_items.empty())
            neighbour = _items.front().root;
        for (std::size_t position = 0; neighbour != nullptr && position < _container.children.size(); ++position)
        {
            if (_container.children[position].get() == neighbour)
                return position + shift;
        }
        return _container.children.size();
    }
}
