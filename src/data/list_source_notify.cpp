#include "gran/data/internal/list_notify.hpp"

namespace gran::data
{
    // Пустые изменения не рассылаются: подписчику нечего обновлять / Empty changes are not delivered: the subscriber has nothing to update
    void ListSource::notifyInserted(std::size_t index, std::size_t count)
    {
        if (count > 0)
            internal::deliver(*this, _observers, {internal::ListEventKind::INSERTED, index, count});
    }

    void ListSource::notifyRemoved(std::size_t index, std::size_t count)
    {
        if (count > 0)
            internal::deliver(*this, _observers, {internal::ListEventKind::REMOVED, index, count});
    }

    void ListSource::notifyChanged(std::size_t first, std::size_t count)
    {
        if (count > 0)
            internal::deliver(*this, _observers, {internal::ListEventKind::CHANGED, first, count});
    }

    // Перемещение на то же место — не изменение / A move to the same place is not a change
    void ListSource::notifyMoved(std::size_t from, std::size_t count, std::size_t to)
    {
        if (count > 0 && from != to)
            internal::deliver(*this, _observers, {internal::ListEventKind::MOVED, from, count, to});
    }

    void ListSource::notifyReordered(const std::vector<std::size_t>& oldToNew)
    {
        if (!oldToNew.empty())
            internal::deliver(*this, _observers, {internal::ListEventKind::REORDERED, 0, oldToNew.size(), 0, &oldToNew});
    }
}
