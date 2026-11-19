#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    namespace
    {
        /*
        ** Декоратор встаёт поверх прежнего фильтра один раз, дальше меняются только его условия. Все условия стёрты —
        ** возвращается прежний фильтр: строка фильтров без текста не стоит ничего и не держит лишний декоратор.
        ** Условий нет и декоратора нет — фильтр не трогается (пересчёт без нужды сдвинул бы ленты и раскрытые ветки).
        **
        ** The decorator stands over the former filter once, after that only its conditions change. All the conditions erased —
        ** the former filter comes back: a filter row without text costs nothing and keeps no extra decorator.
        ** No conditions and no decorator — the filter is not touched (a needless recomputation would move rows and expanded branches).
        */
        void filterProxy(data::SortFilterProxy& proxy, const std::vector<data::ColumnCondition>& conditions)
        {
            std::shared_ptr<data::ColumnFilter> filter = std::dynamic_pointer_cast<data::ColumnFilter>(proxy.filter());

            if (!filter && conditions.empty())
                return;
            if (!filter)
                filter = std::make_shared<data::ColumnFilter>(proxy.filter(), nullptr);
            filter->setConditions(conditions);
            if (filter->empty())
                proxy.setFilter(filter->baseRecords());
            else
                proxy.setFilter(filter);
        }

        // То же для плоского дерева: предки подходящих узлов остаются видны и раскрываются / The same for a flat tree: the ancestors of the matching nodes stay visible and expand
        void filterTree(data::TreeList& tree, const std::vector<data::ColumnCondition>& conditions)
        {
            std::shared_ptr<data::ColumnFilter> filter = std::dynamic_pointer_cast<data::ColumnFilter>(tree.filter());

            if (!filter && conditions.empty())
                return;
            if (!filter)
                filter = std::make_shared<data::ColumnFilter>(nullptr, tree.filter());
            filter->setConditions(conditions);
            if (filter->empty())
                tree.setFilter(filter->baseNodes());
            else
                tree.setFilter(filter);
        }
    }

    bool filtersColumns(data::ListSource& model) noexcept
    {
        return dynamic_cast<data::SortFilterProxy*>(&model) != nullptr || dynamic_cast<data::TreeList*>(&model) != nullptr;
    }

    void applyColumnConditions(data::ListSource& model, const std::vector<data::ColumnCondition>& conditions)
    {
        data::SortFilterProxy* proxy = dynamic_cast<data::SortFilterProxy*>(&model);
        data::TreeList* tree = dynamic_cast<data::TreeList*>(&model);

        if (proxy != nullptr)
            filterProxy(*proxy, conditions);
        else if (tree != nullptr)
            filterTree(*tree, conditions);
    }

    bool replaceColumnFilterBase(data::SortFilterProxy* rows, data::TreeList* nodes,
        const std::shared_ptr<ExpressionFilter>& filter)
    {
        std::shared_ptr<data::ColumnFilter> current;

        if (rows != nullptr)
            current = std::dynamic_pointer_cast<data::ColumnFilter>(rows->filter());
        else if (nodes != nullptr)
            current = std::dynamic_pointer_cast<data::ColumnFilter>(nodes->filter());
        if (!current)
            return false;
        current->setBase(filter, filter);
        if (rows != nullptr)
            rows->invalidateFilter();
        else
            nodes->invalidateFilter();
        return true;
    }
}
