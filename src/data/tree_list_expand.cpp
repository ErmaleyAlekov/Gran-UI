#include "gran/data/internal/list_notify.hpp"
#include "gran/data/tree_list.hpp"

namespace gran::data
{
    // Строк на экране и в раскрытых ветках немного — линейный поиск / There are few rows on screen and in the expanded branches — a linear search
    std::size_t TreeList::rowOf(NodeId node) const noexcept
    {
        for (std::size_t row = 0; row < _rows.size(); ++row)
        {
            if (_rows[row].node == node)
                return row;
        }
        return static_cast<std::size_t>(-1);
    }

    /*
    ** Признак ставится до подгрузки: дети ленивого узла, вставленные подгрузчиком, сразу появляются
    ** раскрытыми строками (каждая вставка — своё уведомление inserted).
    **
    ** The flag is set before fetching: the lazy node children inserted by the fetcher appear at once
    ** as expanded rows (each insertion is its own inserted notification).
    */
    void TreeList::expand(std::size_t row)
    {
        NodeId id = node(row);

        if (_rows[row].expanded || !_rows[row].hasChildren)
            return;
        _expanded.insert(id);
        if (_tree.lazy(id))
            _tree.fetchChildren(id);
        refresh();
    }

    void TreeList::collapse(std::size_t row)
    {
        NodeId id = node(row);

        if (_expanded.erase(id) > 0)
            refresh();
    }

    void TreeList::toggle(std::size_t row)
    {
        if (expanded(row))
            collapse(row);
        else
            expand(row);
    }
}
