#include "gran/binding/internal/list_binding.hpp"

#include <utility>

namespace gran::binding
{
    /*
    ** Копия шаблона заполняется до вставки (ещё вне дерева — без лишних пометок предков),
    ** затем встаёт на место записи; вложенные списки копии привязываются уже в дереве.
    **
    ** The template copy is filled before insertion (still outside the tree — without extra ancestor marks),
    ** then it takes the record place; the nested lists of the copy are bound already in the tree.
    */
    void ListBinding::materialize(std::size_t record)
    {
        std::size_t local = record - _first;
        std::unique_ptr<ui::Node> copy = ui::cloneTree(*_container.itemTemplate);
        BoundItem item{copy.get(), collectFields(*copy)};

        applyFields(item.fields, _model.at(record));
        std::size_t position = childPosition(local);
        _items.insert(_items.begin() + static_cast<std::ptrdiff_t>(local), std::move(item));
        ui::Node& added = ui::insertChild(_container, position, std::move(copy));
        ui::markDirty(added, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        _owner.bind(added);
    }

    // Узел удаляет владелец дерева: он снимает вложенные привязки и ссылки ввода / The tree owner removes the node: it drops the nested bindings and the input references
    void ListBinding::dematerialize(std::size_t local)
    {
        ui::Node* root = _items[local].root;

        _items.erase(_items.begin() + static_cast<std::ptrdiff_t>(local));
        _owner.removeNode(*root);
    }

    /*
    ** Распорки — обычные Box после собственных детей контейнера: копии окна встают между ними.
    ** Пока перед окном (или после него) нет записей, распорка скрыта и не добавляет промежутка.
    **
    ** The spacers are ordinary Boxes after the container's own children: the window copies go between them.
    ** While there are no records before (or after) the window, the spacer is hidden and adds no gap.
    */
    void ListBinding::createSpacers()
    {
        std::unique_ptr<ui::Node> lead = std::make_unique<ui::Node>();
        std::unique_ptr<ui::Node> trail = std::make_unique<ui::Node>();

        lead->visible = false;
        trail->visible = false;
        _lead = &ui::appendChild(_container, std::move(lead));
        _trail = &ui::appendChild(_container, std::move(trail));
        ui::markDirty(_container, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }
}
