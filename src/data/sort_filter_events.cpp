#include "gran/data/internal/list_notify.hpp"
#include "gran/data/sort_filter_proxy.hpp"

namespace gran::data
{
    // Сначала сдвиг номеров прежних записей, затем каждая новая встаёт на своё место / First the indices of the former records shift, then each new one takes its place
    void SortFilterProxy::inserted(std::size_t index, std::size_t count)
    {
        for (std::size_t& source : _map)
        {
            if (source >= index)
                source += count;
        }
        _reverseValid = false;
        for (std::size_t source = index; source < index + count; ++source)
        {
            if (accepts(source))
                place(source);
        }
    }

    // Номера оставшихся записей сдвигаются до уведомлений: подписчик читает уже верные записи / The indices of the remaining records shift before the notifications: the subscriber reads the right records already
    void SortFilterProxy::removed(std::size_t index, std::size_t count)
    {
        std::vector<std::size_t> rows;

        for (std::size_t row = _map.size(); row > 0; --row)
        {
            std::size_t& source = _map[row - 1];
            if (source >= index && source < index + count)
                rows.push_back(row - 1);
            else if (source >= index + count)
                source -= count;
        }
        eraseRows(rows);
    }

    /*
    ** Правка поля может спрятать или показать запись и сдвинуть её по ключам: строка переезжает
    ** (moved — фокус и выделение едут вместе с ней), затем получает changed с новыми полями.
    **
    ** A field edit may hide or show the record and shift it by the keys: the row moves
    ** (moved — the focus and the selection travel with it), then it gets changed with the new fields.
    */
    void SortFilterProxy::changed(std::size_t index)
    {
        std::size_t row = proxyIndex(index);
        bool shown = accepts(index);

        if (row == static_cast<std::size_t>(-1))
        {
            if (shown)
                place(index);
            return;
        }
        if (!shown)
        {
            eraseRows({row});
            return;
        }
        _map.erase(_map.begin() + static_cast<std::ptrdiff_t>(row));
        std::size_t target = position(index);
        _map.insert(_map.begin() + static_cast<std::ptrdiff_t>(target), index);
        _reverseValid = false;
        notifyMoved(row, 1, target);
        notifyChanged(target, 1);
    }

    // Перемещение в источнике — частный случай перестановки / A move in the source is a special case of a permutation
    void SortFilterProxy::moved(std::size_t from, std::size_t count, std::size_t to)
    {
        reordered(internal::moveMapping(_source.size(), from, count, to));
    }

    // Равные по ключам записи следуют новому порядку источника / Records equal by the keys follow the new source order
    void SortFilterProxy::reordered(const std::vector<std::size_t>& oldToNew)
    {
        for (std::size_t& source : _map)
        {
            if (source < oldToNew.size())
                source = oldToNew[source];
        }
        _reverseValid = false;
        resort();
    }
}
