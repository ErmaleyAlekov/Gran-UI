#include "gran/binding/internal/expression_binding.hpp"

namespace gran::binding
{
    void ExpressionBindings::watchModel(const std::string& name, data::ListSource& model)
    {
        _mirror.watch(name, model);
    }

    void ExpressionBindings::watchTree(const std::string& name, data::TreeModel& tree)
    {
        _mirror.watchTree(name, tree);
    }

    void ExpressionBindings::watchSelection(const std::string& name, data::SelectionModel& selection)
    {
        _mirror.watchSelection(name, selection);
    }

    std::shared_ptr<ExpressionFilter> ExpressionBindings::makeFilter(const std::string& expression)
    {
        std::shared_ptr<ExpressionFilter> filter = std::make_shared<ExpressionFilter>(*this, expression);

        _filters.push_back(filter);
        return filter;
    }

    // Заменённые фильтры (их представление держит уже другой) выбывают из списка / Replaced filters (their view holds another one already) leave the list
    void ExpressionBindings::refreshFilters()
    {
        for (std::size_t index = 0; index < _filters.size();)
        {
            std::shared_ptr<ExpressionFilter> filter = _filters[index].lock();
            if (filter == nullptr)
            {
                _filters.erase(_filters.begin() + static_cast<std::ptrdiff_t>(index));
                continue;
            }
            if (filter->changed())
                filter->invalidate();
            ++index;
        }
    }
}
