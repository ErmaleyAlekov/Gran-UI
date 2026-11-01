#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran
{
    data::TreeModel& Application::tree(const std::string& name)
    {
        return _state->lists.tree(name);
    }

    data::ListSource& Application::list(const std::string& name)
    {
        data::ListSource* found = _state->lists.list(name);

        if (found == nullptr)
            return _state->lists.model(name);
        return *found;
    }

    data::SortFilterProxy& Application::createProxy(const std::string& name, const std::string& source)
    {
        return _state->lists.createProxy(name, source);
    }

    data::TreeList& Application::createTreeList(const std::string& name, const std::string& tree)
    {
        return _state->lists.createTreeList(name, tree);
    }
}
