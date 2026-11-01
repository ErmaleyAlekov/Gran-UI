#include "gran/data/list_source.hpp"

namespace gran::data
{
    void ListObserver::changedRange(std::size_t first, std::size_t count)
    {
        for (std::size_t index = first; index < first + count; ++index)
            changed(index);
    }

    void ListObserver::moved(std::size_t from, std::size_t count, std::size_t to)
    {
        removed(from, count);
        inserted(to, count);
    }

    void ListObserver::reordered(const std::vector<std::size_t>& oldToNew)
    {
        removed(0, oldToNew.size());
        inserted(0, oldToNew.size());
    }

    ListSource::~ListSource() = default;

    // Общий пустой набор: у списка без столбцов нечего хранить / A shared empty set: a list without columns has nothing to keep
    const ColumnSet& ListSource::columns() const noexcept
    {
        static const ColumnSet EMPTY;

        return EMPTY;
    }
}
