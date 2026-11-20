#include "gran/platform/win32/internal/uia_element.hpp"
#include "gran/ui/data_grid.hpp"

namespace gran::platform::win32
{
    /*
    ** Строка — копия записи (её номер знают сведения о строках), ячейка — её ребёнок со своим столбцом.
    ** Заголовки и строка фильтров — не строки списка: ключ пустой, элемент живёт по узлу.
    **
    ** A row is a record copy (the row facts know its index), a cell is its child with its own column.
    ** The headers and the filter row are not list rows: the key is empty, the element lives by the node.
    */
    ItemKey itemKeyOf(const UiaHost& host, const ui::Node* node) noexcept
    {
        ItemKey key;
        const ui::Node* row = node;

        if (node == nullptr || host.tree == nullptr || host.tree->items == nullptr)
            return key;
        key.role = a11y::roleOf(*node);
        if (key.role == a11y::Role::CELL)
            row = node->parent;
        else if (key.role != a11y::Role::DATA_ITEM && key.role != a11y::Role::TREE_ITEM)
            return key;
        if (row == nullptr || row->parent == nullptr)
            return key;
        std::size_t record = host.tree->items->itemIndex(*row);
        if (record == a11y::NO_ITEM)
            return key;
        if (key.role == a11y::Role::CELL)
            key.column = ui::cellColumn(*row, *node);
        key.list = row->parent;
        key.listSerial = key.list->serial;
        key.record = record;
        key.version = host.tree->items->itemVersion(*key.list);
        return key;
    }

    // Список тот же (адрес в дереве и номер) — тогда его копия записи и есть этот элемент / The list is the same (the address in the tree and the serial) — then its record copy is this element
    bool UiaElement::rebind() const noexcept
    {
        const a11y::ItemSource* items = _host->tree->items;

        if (_key.list == nullptr || items == nullptr || !a11y::contains(*_host->tree, _key.list)
            || _key.list->serial != _key.listSerial || items->itemVersion(*_key.list) != _key.version)
            return false;
        const ui::Node* found = items->itemAt(*_key.list, _key.record);
        if (found != nullptr && _key.column != static_cast<std::size_t>(-1))
            found = ui::gridCell(const_cast<ui::Node&>(*found), _key.column);
        if (found == nullptr || !a11y::contains(*_host->tree, found))
            return false;
        _node = found;
        _serial = found->serial;
        return true;
    }

    // Ключ строки: тот же живой список той же версии, запись ещё в нём / The row key: the same alive list of the same version, the record is still in it
    bool UiaElement::virtualRow() const noexcept
    {
        const a11y::ItemSource* items = nullptr;

        if (_host->tree == nullptr || _key.list == nullptr || _key.column != static_cast<std::size_t>(-1))
            return false;
        items = _host->tree->items;
        if (items == nullptr || !a11y::contains(*_host->tree, _key.list) || _key.list->serial != _key.listSerial
            || items->itemVersion(*_key.list) != _key.version)
            return false;
        return _key.record < items->itemCount(*_key.list);
    }

    // Запись ещё в списке, но копии нет — строка вне видимой части виртуального списка / The record is still in the list but has no copy — the row is outside the visible part of a virtual list
    Presence UiaElement::presence() const noexcept
    {
        if (alive())
            return Presence::ALIVE;
        if (virtualRow())
            return Presence::VIRTUAL;
        return Presence::GONE;
    }
}
