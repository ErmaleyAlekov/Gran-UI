#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"
#include "gran/binding/internal/reload_checkpoint.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/data/internal/observer_errors.hpp"

#include <utility>

namespace gran::binding
{
    namespace
    {
        bool createsCycle(data::SortFilterProxy* target, data::ListSource* base)
        {
            while (auto* proxy = dynamic_cast<data::SortFilterProxy*>(base))
            {
                if (proxy == target)
                    return true;
                base = &proxy->source();
            }
            return false;
        }
    }

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
                data::internal::ObserverErrors::checkCurrent();
                target.setColumns(std::move(set.columns));
                target.insertAll(0, std::move(set.records));
                target.setFetcher(std::move(set.fetcher));
            }
            else if (set.kind == markup::DataKind::TREE)
            {
                data::TreeModel& target = tree(set.name);
                target.clear();
                data::internal::ObserverErrors::checkCurrent();
                data::fillTree(target, target.root(), std::move(set.nodes));
            }
            data::internal::ObserverErrors::checkCurrent();
        }
        adoptViews(sets);
        sets.clear();
    }

    void ListBindings::adoptViews(const std::vector<markup::DataSet>& sets)
    {
        std::vector<const markup::DataSet*> pending;
        for (const auto& set : sets)
            if (set.kind != markup::DataKind::LIST && set.kind != markup::DataKind::TREE)
                pending.push_back(&set);
        while (!pending.empty())
        {
            const auto count = pending.size();
            for (auto it = pending.begin(); it != pending.end();)
            {
                const auto& set = **it;
                if (set.kind == markup::DataKind::SORT_FILTER && createsCycle(proxy(set.name), list(set.source)))
                    ++it;
                else
                {
                    adoptView(set);
                    it = pending.erase(it);
                }
            }
            if (pending.size() == count)
                throw GranException(ErrorCode::INVALID_ARGUMENT,
                    i18n::format(i18n::Message::DATA_SOURCE_CYCLE, {pending.front()->name}));
        }
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
        {
            auto& view = *proxy(set.name);
            auto* base = list(set.source);
            if (!base)
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_UNKNOWN_MODEL, {set.source}));
            if (&view.source() != base && _reload) _reload->rememberSource(view);
            view.replaceSource(*base);
            view.setSort(data::parseSortKeys(set.sort));
        }
        if (set.kind == markup::DataKind::TREE_LIST)
        {
            auto& view = *treeList(set.name);
            const auto base = _trees.find(set.source);
            if (base == _trees.end())
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_UNKNOWN_MODEL, {set.source}));
            if (&view.tree() != base->second.get() && _reload) _reload->rememberSource(view);
            view.replaceSource(*base->second);
            view.setSort(data::parseSortKeys(set.sort));
        }
        if (set.kind == markup::DataKind::SORT_FILTER || set.kind == markup::DataKind::TREE_LIST)
            setFilterExpression(set.name, set.filter);
        if (set.kind == markup::DataKind::TREE_LIST)
            treeList(set.name)->expandToDepth(set.expanded);
    }
}
