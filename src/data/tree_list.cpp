#include "gran/data/internal/list_notify.hpp"
#include "gran/data/tree_list.hpp"

#include <utility>

namespace gran::data
{
    // Строки строятся до подписки: уведомлять пока некого / The rows are built before the subscription: nobody to notify yet
    TreeList::TreeList(TreeModel& tree)
        : _tree(tree)
    {
        _rows = build();
        _tree.subscribe(this);
    }

    TreeList::~TreeList()
    {
        _tree.unsubscribe(this);
    }

    std::size_t TreeList::size() const noexcept
    {
        return _rows.size();
    }

    const Record& TreeList::at(std::size_t index) const
    {
        internal::checkIndex(index, _rows.size());
        return _tree.record(_rows[index].node);
    }

    // Дерево разошлёт changed, и представление обновит строку само / The tree delivers changed, and the view updates the row itself
    void TreeList::set(std::size_t index, const std::string& field, std::string value)
    {
        internal::checkIndex(index, _rows.size());
        _tree.set(_rows[index].node, field, std::move(value));
    }
}
