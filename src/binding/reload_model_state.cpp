#include "gran/binding/internal/reload_checkpoint.hpp"
#include "gran/data/column_filter.hpp"

namespace gran::binding
{
    void ReloadCheckpoint::save(data::ListModel& list)
    {
        undo.emplace_back([&list, rows = list._records, columns = list._columns,
                           fetcher = list._fetcher]() mutable {
            list.clear();
            list.setColumns(std::move(columns));
            list.setFetcher(std::move(fetcher));
            list._records.swap(rows);
            list.notifyInserted(0, list.size());
        });
    }

    void ReloadCheckpoint::save(data::TreeModel& tree)
    {
        undo.emplace_back([&tree, nodes = tree._nodes, next = tree._next]() mutable {
            tree.clear();
            tree._nodes.swap(nodes);
            tree._next = next;
            const auto count = tree.childCount(tree.root());
            if (count) tree.notify(data::TreeModel::Event::INSERTED, tree.root(), 0, 0, count);
        });
    }

    void ReloadCheckpoint::save(data::TreeList& view)
    {
        auto columns = std::dynamic_pointer_cast<data::ColumnFilter>(view._filter);
        if (columns)
            undo.emplace_back([columns, saved = *columns]() mutable { *columns = std::move(saved); });
        undo.emplace_back([&view, expanded = view._expanded, keys = view._keys,
                           filter = view._filter]() mutable {
            view._expanded.swap(expanded);
            view._keys.swap(keys);
            view._filter.swap(filter);
            view.refresh();
        });
    }

    void ReloadCheckpoint::save(data::SortFilterProxy& proxy)
    {
        auto columns = std::dynamic_pointer_cast<data::ColumnFilter>(proxy.filter());
        if (columns)
            undo.emplace_back([columns, saved = *columns]() mutable { *columns = std::move(saved); });
        undo.emplace_back([&proxy, keys = proxy.sort(), filter = proxy.filter()]() mutable {
            proxy.setSort(std::move(keys));
            proxy.setFilter(std::move(filter));
        });
    }

    void ReloadCheckpoint::save(data::SelectionModel& model)
    {
        undo.emplace_back([&model, marks = model._marks, keys = model._keys, hidden = model._hidden,
                           mode = model._mode, count = model._count, current = model._current,
                           anchor = model._anchor]() mutable {
            model._marks.swap(marks);
            model._keys.swap(keys);
            model._hidden.swap(hidden);
            model._mode = mode;
            model._count = count;
            model._current = current;
            model._anchor = anchor;
            model.publish();
        });
    }
}
