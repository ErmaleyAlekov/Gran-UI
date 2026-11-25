#include "gran/binding/internal/reload_checkpoint.hpp"
#include "gran/binding/internal/expression_binding.hpp"

namespace gran::binding
{
    ReloadCheckpoint::ReloadCheckpoint(ListBindings& lists, const markup::UiDocument& next) : owner(lists)
    {
        saveRegistry(lists);
        for (const auto& set : next.data)
        {
            auto list = lists._models.find(set.name);
            auto tree = lists._trees.find(set.name);
            if (list != lists._models.end()) save(*list->second);
            if (tree != lists._trees.end()) save(*tree->second);
        }
        for (auto& view : lists._views)
        {
            if (auto* flat = lists.treeList(view.name)) save(*flat);
            if (auto* proxy = lists.proxy(view.name)) save(*proxy);
        }
        for (auto& selection : lists._selections) save(*selection.second);
        lists._reload = this;
    }

    void ReloadCheckpoint::check() const { errors.check(); }

    void ReloadCheckpoint::restore()
    {
        for (auto it = undoSources.rbegin(); it != undoSources.rend(); ++it) (*it)();
        for (auto& action : undo) action();
    }

    void ReloadCheckpoint::rememberSource(data::SortFilterProxy& proxy)
    {
        undoSources.emplace_back([&proxy, &base = proxy.source()] { proxy.replaceSource(base); });
    }

    void ReloadCheckpoint::rememberSource(data::TreeList& view)
    {
        undoSources.emplace_back([&view, &base = view.tree()] { view.replaceSource(base); });
    }
}
