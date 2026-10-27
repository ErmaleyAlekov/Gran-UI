#ifndef GRAN_DATA_INTERNAL_LIST_NOTIFY_HPP
#define GRAN_DATA_INTERNAL_LIST_NOTIFY_HPP

#include "gran/data/list_model.hpp"

namespace gran::data::internal
{
    // Вид изменения модели / The model change kind
    enum class ListEventKind { INSERTED, REMOVED, CHANGED };

    // Одно изменение модели для рассылки подписчикам / One model change to deliver to the subscribers
    struct ListEvent
    {
        ListEventKind kind;
        std::size_t index;
        std::size_t count;
    };

    // ---- list_notify.cpp ----

    // Номер вне [0, size) → GranException(INVALID_ARGUMENT) / An index outside [0, size) → GranException(INVALID_ARGUMENT)
    void checkIndex(std::size_t index, std::size_t size);

    /*
    ** Рассылка по копии списка подписчиков: подписчик может отписать себя или другого
    ** (удаление узлов списка уничтожает вложенные привязки). Отписанный во время
    ** рассылки больше не получает уведомлений.
    **
    ** Delivering over a copy of the subscriber list: a subscriber may unsubscribe itself or another one
    ** (removing list nodes destroys the nested bindings). One unsubscribed during
    ** the delivery gets no more notifications.
    */
    void deliver(const ListModel& model, std::vector<ListObserver*> observers, const ListEvent& event);
}

#endif
