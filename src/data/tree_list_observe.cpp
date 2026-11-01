#include "gran/data/tree_list.hpp"

namespace gran::data
{
    /*
    ** Место правки в дереве само по себе не важно: строки пересчитываются по дереву целиком, а наружу
    ** уходит только разница. Узлы, удалённые из дерева, заодно забываются в наборе раскрытых.
    **
    ** The place of an edit in the tree does not matter by itself: the rows are recomputed over the whole tree, and only
    ** the difference goes out. The nodes removed from the tree are also forgotten in the expanded set.
    */
    void TreeList::inserted(NodeId, std::size_t, std::size_t)
    {
        refresh();
    }

    void TreeList::removed(NodeId, std::size_t, std::size_t)
    {
        for (auto node = _expanded.begin(); node != _expanded.end();)
        {
            if (_tree.contains(*node))
                ++node;
            else
                node = _expanded.erase(node);
        }
        refresh();
    }

    void TreeList::moved(NodeId, NodeId, std::size_t)
    {
        refresh();
    }

    // Правка полей не меняет строк без сортировки и фильтра, но сама строка обновляется / A field edit does not change the rows without sorting and a filter, but the row itself is updated
    void TreeList::changed(NodeId node)
    {
        std::size_t row = 0;

        refresh();
        row = rowOf(node);
        if (row != static_cast<std::size_t>(-1))
            notifyChanged(row, 1);
    }
}
