#include "gran/data/internal/list_notify.hpp"
#include "gran/data/tree_model.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <string>
#include <utility>

namespace gran::data
{
    /*
    ** Узел не может стать потомком самого себя: такой перенос оторвал бы ветку от корня.
    ** row — место в новом порядке детей newParent (после изъятия узла со старого места).
    **
    ** A node cannot become a descendant of itself: such a move would tear the branch off the root.
    ** row is the place in the new order of the newParent children (after taking the node from the old place).
    */
    void TreeModel::move(NodeId id, NodeId newParent, std::size_t row)
    {
        NodeId oldParent = parent(id);
        std::size_t oldRow = this->row(id);

        for (NodeId current = newParent; current != 0; current = parent(current))
        {
            if (current == id || oldParent == 0)
                throw GranException(ErrorCode::INVALID_ARGUMENT,
                    i18n::format(i18n::Message::DATA_TREE_CYCLE, {std::to_string(id)}));
        }
        // Место проверяется до изъятия: при ошибке дерево остаётся прежним / The place is checked before taking out: on an error the tree stays as it was
        std::size_t room = childCount(newParent);
        if (oldParent == newParent)
            --room;
        internal::checkIndex(row, room + 1);
        if (oldParent == newParent && oldRow == row)
            return;
        std::vector<NodeId>& from = _nodes[oldParent].children;
        from.erase(from.begin() + static_cast<std::ptrdiff_t>(oldRow));
        std::vector<NodeId>& to = _nodes[newParent].children;
        to.insert(to.begin() + static_cast<std::ptrdiff_t>(row), id);
        _nodes[id].parent = newParent;
        notify(Event::MOVED, id, oldParent, oldRow, 1);
    }

    void TreeModel::clear()
    {
        clearChildren(root());
    }

    void TreeModel::setLazy(NodeId id, bool lazy)
    {
        if (node(id).lazy == lazy)
            return;
        _nodes[id].lazy = lazy;
        notify(Event::CHANGED, id, 0, 0, 0);
    }

    void TreeModel::setFetcher(std::shared_ptr<TreeFetcher> fetcher)
    {
        _fetcher = std::move(fetcher);
    }

    // Признак снимается до подгрузки: подгрузчик может вставить детей, а может и не найти их / The flag is dropped before fetching: the fetcher may insert children or find none
    void TreeModel::fetchChildren(NodeId id)
    {
        std::shared_ptr<TreeFetcher> fetcher = _fetcher;

        if (!lazy(id))
            return;
        setLazy(id, false);
        if (fetcher != nullptr)
            fetcher->fetchChildren(*this, id);
    }
}
