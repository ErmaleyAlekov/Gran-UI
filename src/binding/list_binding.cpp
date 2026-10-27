#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    /*
    ** Обычный список строит копии всех записей сразу (модель могла быть заполнена до загрузки
    ** разметки); виртуальный — распорки и начальную порцию, остальное — после первой компоновки.
    **
    ** An ordinary list builds the copies of all records at once (the model may have been filled before
    ** the markup was loaded); a virtual one — the spacers and the initial portion, the rest — after the first layout.
    */
    ListBinding::ListBinding(ListBindings& owner, ui::Node& container, data::ListModel& model)
        : _owner(owner), _container(container), _model(model)
    {
        _virtual = container.content.virtualize;
        if (_virtual)
            createSpacers();
        _model.subscribe(this);
        inserted(0, _model.size());
    }

    // Узлы не трогаются: их удаляет владелец дерева / The nodes are not touched: the tree owner removes them
    ListBinding::~ListBinding()
    {
        _model.unsubscribe(this);
    }

    /*
    ** Запись перед окном сдвигает окно, внутри окна — получает копию, сразу за окном — копию,
    ** пока окно не заполнено (у обычного списка оно не заполняется никогда), дальше — только распорка.
    **
    ** A record before the window shifts the window, inside the window — gets a copy, right after the window — a copy
    ** while the window is not full (an ordinary list never fills it), further on — only the spacer.
    */
    void ListBinding::inserted(std::size_t index, std::size_t count)
    {
        for (std::size_t record = index; record < index + count; ++record)
        {
            std::size_t end = _first + _items.size();
            bool room = !_virtual || _items.size() < _capacity;
            if (record < _first)
                ++_first;
            else if (record < end || (record == end && room))
                materialize(record);
        }
        applySpacers();
    }

    // С конца, чтобы номера ещё не удалённых записей не сдвигались / From the end, so that the indices of the records not yet removed do not shift
    void ListBinding::removed(std::size_t index, std::size_t count)
    {
        for (std::size_t record = index + count; record > index; --record)
        {
            if (record - 1 < _first)
                --_first;
            else if (record - 1 < _first + _items.size())
                dematerialize(record - 1 - _first);
        }
        applySpacers();
    }

    // Только тексты с подстановками и только у записи в окне / Only the texts with placeholders and only for a record in the window
    void ListBinding::changed(std::size_t index)
    {
        if (index >= _first && index - _first < _items.size())
            applyFields(_items[index - _first].fields, _model.at(index));
    }
}
