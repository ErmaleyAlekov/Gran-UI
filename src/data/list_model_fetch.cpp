#include "gran/data/list_model.hpp"

#include <utility>

namespace gran::data
{
    bool ListModel::canFetchMore() const
    {
        return _fetcher != nullptr && _fetcher->hasMore();
    }

    /*
    ** Подгрузчик держится локальной копией: подписчик или подгрузчик из кода может сменить его
    ** или очистить модель прямо во время порции. Флаг не даёт виртуальному списку, получившему
    ** inserted, попросить ещё порцию изнутри этой же.
    **
    ** The fetcher is held by a local copy: a subscriber or a fetcher from code may replace it
    ** or clear the model right during the portion. The flag keeps a virtual list that got
    ** inserted from asking for another portion from inside this one.
    */
    void ListModel::fetchMore()
    {
        std::shared_ptr<ListFetcher> fetcher = _fetcher;

        if (_fetching || fetcher == nullptr || !fetcher->hasMore())
            return;
        _fetching = true;
        try
        {
            std::vector<Record> portion = fetcher->next();
            if (!portion.empty())
                insertAll(_records.size(), std::move(portion));
        }
        catch (...)
        {
            _fetching = false;
            throw;
        }
        _fetching = false;
    }
}
