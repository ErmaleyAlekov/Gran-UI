#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    // Модель живёт в куче: ссылки на неё стабильны при росте словаря / The model lives on the heap: references to it are stable when the map grows
    data::ListModel& ListBindings::model(const std::string& name)
    {
        auto found = _models.find(name);

        if (found != _models.end())
            return *found->second;
        checkName(name, "list");
        std::unique_ptr<data::ListModel>& slot = _models[name];
        slot = std::make_unique<data::ListModel>();
        _expressions->watchModel(name, *slot);
        return *slot;
    }

    data::TreeModel& ListBindings::tree(const std::string& name)
    {
        auto found = _trees.find(name);

        if (found != _trees.end())
            return *found->second;
        checkName(name, "tree");
        std::unique_ptr<data::TreeModel>& slot = _trees[name];
        slot = std::make_unique<data::TreeModel>();
        _expressions->watchTree(name, *slot);
        return *slot;
    }

    // Представлений единицы — линейный поиск / There are few views — a linear search
    data::ListSource* ListBindings::list(std::string_view name) const noexcept
    {
        auto found = _models.find(name);

        if (found != _models.end())
            return found->second.get();
        for (const View& view : _views)
        {
            if (view.name == name)
                return view.source.get();
        }
        return nullptr;
    }

    data::SelectionModel* ListBindings::selection(std::string_view name) const noexcept
    {
        auto found = _selections.find(name);

        if (found == _selections.end())
            return nullptr;
        return found->second.get();
    }

    std::string ListBindings::nameOf(const data::ListSource& source) const
    {
        for (const auto& entry : _models)
        {
            if (entry.second.get() == &source)
                return entry.first;
        }
        for (const View& view : _views)
        {
            if (view.source.get() == &source)
                return view.name;
        }
        return std::string();
    }
}
