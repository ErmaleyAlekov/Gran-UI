#include "gran/data/internal/source_readers.hpp"
#include "gran/data/record_order.hpp"

#include <utility>

namespace gran::data
{
    // Расширение сравнивается без учёта регистра латиницы (DATA.JSON) / The extension is compared ignoring the Latin case (DATA.JSON)
    SourceFormat sourceFormat(const SourceSpec& spec)
    {
        std::string extension;

        if (spec.format != SourceFormat::AUTO)
            return spec.format;
        if (spec.path.size() >= 5)
            extension = foldCase(spec.path.substr(spec.path.size() - 5));
        if (extension == ".json")
            return SourceFormat::JSON;
        return SourceFormat::XML;
    }

    std::shared_ptr<ListFetcher> openListSource(const SourceSpec& spec)
    {
        if (sourceFormat(spec) == SourceFormat::JSON)
            return std::make_shared<internal::JsonListFetcher>(spec);
        return std::make_shared<internal::XmlListFetcher>(spec);
    }

    std::vector<TreeItem> readTreeSource(const SourceSpec& spec)
    {
        if (sourceFormat(spec) == SourceFormat::JSON)
            return internal::readJsonTree(spec);
        return internal::readXmlTree(spec);
    }

    // Подгрузчик ставится после clear (clear его снимает) и только если записи ещё остались / The fetcher is set after clear (clear drops it) and only if records remain
    void loadList(ListModel& model, const SourceSpec& spec)
    {
        std::shared_ptr<ListFetcher> fetcher = openListSource(spec);
        std::vector<Record> first = fetcher->next();

        model.clear();
        model.insertAll(0, std::move(first));
        if (fetcher->hasMore())
            model.setFetcher(std::move(fetcher));
    }

    // Одним пакетом: подписчики пересчитываются один раз / As one batch: the subscribers recompute once
    void fillTree(TreeModel& tree, NodeId parent, std::vector<TreeItem> items)
    {
        tree.appendItems(parent, std::move(items));
    }
}
