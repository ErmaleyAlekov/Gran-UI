#ifndef GRAN_DATA_INTERNAL_LIST_NOTIFY_HPP
#define GRAN_DATA_INTERNAL_LIST_NOTIFY_HPP

#include "gran/data/list_source.hpp"

namespace gran::data::internal
{
    // Вид изменения списка / The list change kind
    enum class ListEventKind { INSERTED, REMOVED, CHANGED, MOVED, REORDERED };

    // Одно изменение списка для рассылки подписчикам / One list change to deliver to the subscribers
    struct ListEvent
    {
        ListEventKind kind;
        std::size_t index;
        std::size_t count;
        // Куда переехали записи (MOVED) и перестановка (REORDERED) / Where the records went (MOVED) and the permutation (REORDERED)
        std::size_t to = 0;
        const std::vector<std::size_t>* mapping = nullptr;
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
    void deliver(const ListSource& source, std::vector<ListObserver*> observers, const ListEvent& event);

    // Перестановка oldToNew для списка из size записей после moved(from, count, to) / The oldToNew permutation for a list of size records after moved(from, count, to)
    std::vector<std::size_t> moveMapping(std::size_t size, std::size_t from, std::size_t count, std::size_t to);
}

#endif
