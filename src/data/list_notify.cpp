#include "gran/data/internal/list_notify.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <string>
#include <utility>

namespace gran::data::internal
{
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
    void deliver(const ListModel& model, std::vector<ListObserver*> observers, const ListEvent& event)
    {
        for (ListObserver* observer : observers)
        {
            if (!model.subscribed(observer))
                continue;
            if (event.kind == ListEventKind::INSERTED)
                observer->inserted(event.index, event.count);
            else if (event.kind == ListEventKind::REMOVED)
                observer->removed(event.index, event.count);
            else
                observer->changed(event.index);
        }
    }
}
