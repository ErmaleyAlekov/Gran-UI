#include "gran/data/tree_model.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <algorithm>
#include <string>

namespace gran::data
{
    void TreeModel::subscribe(TreeObserver* observer)
    {
        if (observer != nullptr && !subscribed(observer))
            _observers.push_back(observer);
    }

    void TreeModel::unsubscribe(TreeObserver* observer) noexcept
    {
        _observers.erase(std::remove(_observers.begin(), _observers.end(), observer), _observers.end());
    }

    bool TreeModel::subscribed(const TreeObserver* observer) const noexcept
    {
        return std::find(_observers.begin(), _observers.end(), observer) != _observers.end();
    }

    // Сообщение называет номер узла: ошибку в коде приложения легко найти / The message names the node number: the error in the application code is easy to find
    const TreeModel::Node& TreeModel::node(NodeId id) const
    {
        auto found = _nodes.find(id);

        if (found == _nodes.end())
            throw GranException(ErrorCode::INVALID_ARGUMENT,
                i18n::format(i18n::Message::DATA_NO_TREE_NODE, {std::to_string(id)}));
        return found->second;
    }

    // Копия списка: подписчик может отписать себя или другого во время рассылки / A list copy: a subscriber may unsubscribe itself or another one during the delivery
    void TreeModel::notify(Event event, NodeId id, NodeId other, std::size_t row, std::size_t count)
    {
        std::vector<TreeObserver*> observers = _observers;

        for (TreeObserver* observer : observers)
        {
            if (!subscribed(observer))
                continue;
            if (event == Event::INSERTED)
                observer->inserted(id, row, count);
            else if (event == Event::REMOVED)
                observer->removed(id, row, count);
            else if (event == Event::MOVED)
                observer->moved(id, other, row);
            else
                observer->changed(id);
        }
    }
}
