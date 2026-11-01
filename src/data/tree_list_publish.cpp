#include "gran/data/tree_list.hpp"

#include <utility>

namespace gran::data
{
    namespace
    {
        // Нет строки / No row
        constexpr std::size_t NONE = static_cast<std::size_t>(-1);

        // Перестановка, если новые строки — те же узлы в другом порядке; иначе пусто / The permutation if the new rows are the same nodes in another order; otherwise empty
        std::vector<std::size_t> permutation(const std::vector<NodeId>& before, const std::vector<NodeId>& after)
        {
            std::unordered_map<NodeId, std::size_t> place;
            std::vector<std::size_t> oldToNew;

            if (before.size() != after.size() || before.empty())
                return oldToNew;
            for (std::size_t row = 0; row < after.size(); ++row)
                place[after[row]] = row;
            for (NodeId node : before)
            {
                auto found = place.find(node);
                if (found == place.end())
                    return std::vector<std::size_t>();
                oldToNew.push_back(found->second);
            }
            return oldToNew;
        }

        /*
        ** Общее начало и общий конец (не перекрываются) и куда уходят их строки: середина удаляется
        ** и вставляется заново, поэтому её прежние строки — NONE.
        **
        ** The common head and the common tail (not overlapping) and where their rows go: the middle is removed
        ** and inserted anew, so its former rows are NONE.
        */
        std::vector<std::size_t> splice(const std::vector<NodeId>& before, const std::vector<NodeId>& after,
            std::size_t& prefix, std::size_t& suffix)
        {
            std::vector<std::size_t> oldToNew(before.size(), NONE);

            while (prefix < before.size() && prefix < after.size() && before[prefix] == after[prefix])
                ++prefix;
            while (suffix < before.size() - prefix && suffix < after.size() - prefix
                && before[before.size() - 1 - suffix] == after[after.size() - 1 - suffix])
                ++suffix;
            for (std::size_t row = 0; row < prefix; ++row)
                oldToNew[row] = row;
            for (std::size_t row = 0; row < suffix; ++row)
                oldToNew[before.size() - 1 - row] = after.size() - 1 - row;
            return oldToNew;
        }
    }

    /*
    ** Те же узлы в другом порядке — reordered (сортировка братьев). Иначе изменившаяся середина между
    ** общим началом и концом уходит и встаёт заново: раскрытие и вставка дают ровно вставку, свёртка
    ** и удаление — ровно удаление. Затем — changed строкам со сменившимися признаками.
    **
    ** The same nodes in another order — reordered (sorting siblings). Otherwise the changed middle between
    ** the common head and tail goes away and comes anew: an expansion and an insertion give exactly an insertion, a collapse
    ** and a removal — exactly a removal. Then — changed to the rows with changed flags.
    */
    void TreeList::publish(std::vector<Row> rows)
    {
        std::vector<Row> old = std::move(_rows);
        std::vector<NodeId> before;
        std::vector<NodeId> after;
        std::size_t prefix = 0;
        std::size_t suffix = 0;

        for (const Row& row : old)
            before.push_back(row.node);
        for (const Row& row : rows)
            after.push_back(row.node);
        _rows = std::move(rows);
        std::vector<std::size_t> oldToNew = permutation(before, after);
        if (!oldToNew.empty() && before != after)
            notifyReordered(oldToNew);
        if (oldToNew.empty())
        {
            oldToNew = splice(before, after, prefix, suffix);
            notifyRemoved(prefix, before.size() - prefix - suffix);
            notifyInserted(prefix, after.size() - prefix - suffix);
        }
        publishFlags(old, oldToNew);
    }

    void TreeList::publishFlags(const std::vector<Row>& old, const std::vector<std::size_t>& oldToNew)
    {
        for (std::size_t row = 0; row < old.size() && row < oldToNew.size(); ++row)
        {
            std::size_t target = oldToNew[row];
            if (target == NONE || target >= _rows.size())
                continue;
            const Row& now = _rows[target];
            if (now.depth != old[row].depth || now.expanded != old[row].expanded
                || now.hasChildren != old[row].hasChildren)
                notifyChanged(target, 1);
        }
    }
}
