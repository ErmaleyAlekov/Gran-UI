#include "gran/data/internal/list_notify.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <algorithm>
#include <numeric>
#include <string>
#include <utility>

namespace gran::data::internal
{
    namespace
    {
        // Одно событие одному подписчику / One event to one subscriber
        void deliverOne(ListObserver& observer, const ListEvent& event)
        {
            switch (event.kind)
            {
            case ListEventKind::INSERTED:
                observer.inserted(event.index, event.count);
                break;
            case ListEventKind::REMOVED:
                observer.removed(event.index, event.count);
                break;
            case ListEventKind::CHANGED:
                observer.changedRange(event.index, event.count);
                break;
            case ListEventKind::MOVED:
                observer.moved(event.index, event.count, event.to);
                break;
            case ListEventKind::REORDERED:
                observer.reordered(*event.mapping);
                break;
            }
        }
    }

    // Сообщение называет номер и размер, чтобы ошибку в коде приложения было легко найти
    // The message names the index and the size so that the error in the application code is easy to find
    void checkIndex(std::size_t index, std::size_t size)
    {
        if (index < size)
            return;
        throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_INDEX_OUT_OF_RANGE,
            {std::to_string(index), std::to_string(size)}));
    }

    // Подписчик получает событие, только если всё ещё подписан / A subscriber gets the event only while still subscribed
    void deliver(const ListSource& source, std::vector<ListObserver*> observers, const ListEvent& event)
    {
        for (ListObserver* observer : observers)
        {
            if (source.subscribed(observer))
                deliverOne(*observer, event);
        }
    }

    // Тот же поворот, что делает ListModel::move, но над номерами / The same rotation ListModel::move does, but over the indices
    std::vector<std::size_t> moveMapping(std::size_t size, std::size_t from, std::size_t count, std::size_t to)
    {
        std::vector<std::size_t> order(size, 0);
        std::vector<std::size_t> oldToNew(size, 0);
        auto base = order.begin();

        std::iota(order.begin(), order.end(), static_cast<std::size_t>(0));
        if (to > from && to + count <= size)
            std::rotate(base + static_cast<std::ptrdiff_t>(from), base + static_cast<std::ptrdiff_t>(from + count),
                base + static_cast<std::ptrdiff_t>(to + count));
        else if (to < from && from + count <= size)
            std::rotate(base + static_cast<std::ptrdiff_t>(to), base + static_cast<std::ptrdiff_t>(from),
                base + static_cast<std::ptrdiff_t>(from + count));
        for (std::size_t position = 0; position < size; ++position)
            oldToNew[order[position]] = position;
        return oldToNew;
    }
}
