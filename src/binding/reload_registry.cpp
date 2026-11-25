#include "gran/binding/internal/reload_checkpoint.hpp"
#include "gran/binding/internal/expression_binding.hpp"
#include "beresta.hpp"
#include <set>

namespace gran::binding
{
    namespace
    {
        template<class Map>
        std::set<std::string> names(const Map& entries)
        {
            std::set<std::string> result;
            for (const auto& entry : entries) result.insert(entry.first);
            return result;
        }

        template<class Map>
        void removeNew(Map& entries, const std::set<std::string>& saved)
        {
            for (auto it = entries.begin(); it != entries.end();)
            {
                if (saved.count(it->first)) ++it;
                else it = entries.erase(it);
            }
        }
    }

    void ReloadCheckpoint::saveRegistry(ListBindings& lists)
    {
        auto& expressions = *lists._expressions;
        auto& mirror = expressions._mirror;
        originalNames = names(lists._models);
        originalNames.merge(names(lists._trees));
        originalNames.merge(names(lists._selections));
        for (const auto& view : lists._views) originalNames.insert(view.name);
        undo.emplace_back([&lists, &expressions, &mirror, variables = expressions._variables,
                           entries = mirror._entries.size(), children = mirror.root().getChildCount(),
                           models = names(lists._models), trees = names(lists._trees),
                           selections = names(lists._selections), views = lists._views.size()]() mutable {
            expressions._variables.swap(variables);
            for (auto& variable : expressions._variables) variable.second.version = ++expressions._clock;
            mirror._entries.resize(entries);
            auto& root = *mirror._document->getRoot();
            while (root.getChildCount() > children) root.removeChild(root.getChildCount() - 1);
            removeNew(lists._selections, selections);
            while (lists._views.size() > views) lists._views.pop_back();
            removeNew(lists._trees, trees);
            removeNew(lists._models, models);
        });
    }
}
