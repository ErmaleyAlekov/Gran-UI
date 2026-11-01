#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"

#include <utility>

namespace gran::binding
{
    /*
    ** Сначала хранилища и деревья (их содержимое заменяется), затем представления в порядке разметки:
    ** прокси над прокси видит основу, объявленную раньше. Привязок к новым моделям в момент загрузки
    ** ещё нет; прежние представления получают обычные уведомления и обновляются сами.
    **
    ** The storages and the trees first (their contents are replaced), then the views in the markup order:
    ** a proxy over a proxy sees the base declared earlier. There are no bindings to the new models at load time
    ** yet; the former views get ordinary notifications and update themselves.
    */
    void ListBindings::adoptData(std::vector<markup::DataSet>& sets)
    {
        for (markup::DataSet& set : sets)
        {
            if (set.kind == markup::DataKind::LIST)
            {
                data::ListModel& target = model(set.name);
                target.clear();
                target.setColumns(std::move(set.columns));
                target.insertAll(0, std::move(set.records));
                target.setFetcher(std::move(set.fetcher));
            }
            else if (set.kind == markup::DataKind::TREE)
            {
                data::TreeModel& target = tree(set.name);
                target.clear();
                data::fillTree(target, target.root(), std::move(set.nodes));
            }
        }
        for (const markup::DataSet& set : sets)
            adoptView(set);
        sets.clear();
    }

    /*
    ** Представление, уже созданное прежней загрузкой или кодом, перенастраивается (сортировка, фильтр,
    ** режим): привязки других окон к нему остаются целы. Имя другого вида — ошибка checkName.
    **
    ** A view already created by a former load or by code is reconfigured (the sort, the filter,
    ** the mode): the bindings of other windows to it stay intact. A name of another kind is a checkName error.
    */
    void ListBindings::adoptView(const markup::DataSet& set)
    {
        if (set.kind == markup::DataKind::SELECTION && selection(set.name) == nullptr)
            createSelection(set.name, set.source, set.mode);
        else if (set.kind == markup::DataKind::SELECTION)
            selection(set.name)->setMode(set.mode);
        if (set.kind == markup::DataKind::SORT_FILTER && proxy(set.name) == nullptr)
            createProxy(set.name, set.source);
        if (set.kind == markup::DataKind::TREE_LIST && treeList(set.name) == nullptr)
            createTreeList(set.name, set.source);
        if (set.kind == markup::DataKind::SORT_FILTER)
            proxy(set.name)->setSort(data::parseSortKeys(set.sort));
        if (set.kind == markup::DataKind::TREE_LIST)
            treeList(set.name)->setSort(data::parseSortKeys(set.sort));
        if (set.kind == markup::DataKind::SORT_FILTER || set.kind == markup::DataKind::TREE_LIST)
            setFilterExpression(set.name, set.filter);
        if (set.kind == markup::DataKind::TREE_LIST)
            treeList(set.name)->expandToDepth(set.expanded);
    }
}
