#include "gran/data/list_model.hpp"

#include <utility>

namespace gran::data
{
    // Пустая модель не рассылает ничего / An empty model delivers nothing
    void ListModel::clear()
    {
        std::size_t count = _records.size();

        _fetcher.reset();
        _records.clear();
        notifyRemoved(0, count);
    }

    void ListModel::setColumns(ColumnSet columns)
    {
        _columns = std::move(columns);
    }

    const ColumnSet& ListModel::columns() const noexcept
    {
        return _columns;
    }

    void ListModel::setFetcher(std::shared_ptr<ListFetcher> fetcher)
    {
        _fetcher = std::move(fetcher);
    }
}
