#include "gran/binding/internal/list_binding.hpp"

#include <utility>

namespace gran::binding
{
    // Конструктор сразу строит копии всех записей: модель могла быть заполнена до загрузки разметки
    // The constructor builds the copies of all records at once: the model may have been filled before the markup was loaded
    ListBinding::ListBinding(ListBindings& owner, ui::Node& container, data::ListModel& model)
        : _owner(owner), _container(container), _model(model)
    {
        _model.subscribe(this);
        inserted(0, _model.size());
    }

    // Узлы не трогаются: их удаляет владелец дерева / The nodes are not touched: the tree owner removes them
    ListBinding::~ListBinding()
    {
        _model.unsubscribe(this);
    }

    /*
    ** Копия шаблона заполняется до вставки (ещё вне дерева — без лишних пометок предков),
    ** затем встаёт на место записи; вложенные списки копии привязываются уже в дереве.
    **
    ** The template copy is filled before insertion (still outside the tree — without extra ancestor marks),
    ** then it takes the record place; the nested lists of the copy are bound already in the tree.
    */
    void ListBinding::inserted(std::size_t index, std::size_t count)
    {
        for (std::size_t record = index; record < index + count; ++record)
        {
            std::unique_ptr<ui::Node> copy = ui::cloneTree(*_container.itemTemplate);
            BoundItem item{copy.get(), collectFields(*copy)};
            applyFields(item.fields, _model.at(record));
            std::size_t position = childPosition(record);
            _items.insert(_items.begin() + static_cast<std::ptrdiff_t>(record), std::move(item));
            ui::Node& added = ui::insertChild(_container, position, std::move(copy));
            ui::markDirty(added, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            _owner.bind(added);
        }
    }

    // С конца, чтобы номера ещё не удалённых копий не сдвигались / From the end, so that the indices of the copies not yet removed do not shift
    void ListBinding::removed(std::size_t index, std::size_t count)
    {
        for (std::size_t record = index + count; record > index; --record)
        {
            if (record > _items.size())
                continue;
            ui::Node* root = _items[record - 1].root;
            _items.erase(_items.begin() + static_cast<std::ptrdiff_t>(record - 1));
            _owner.removeNode(*root);
        }
    }

    // Только тексты с подстановками: остальное состояние копии не меняется / Only the texts with placeholders: the rest of the copy state does not change
    void ListBinding::changed(std::size_t index)
    {
        if (index < _items.size())
            applyFields(_items[index].fields, _model.at(index));
    }
}
