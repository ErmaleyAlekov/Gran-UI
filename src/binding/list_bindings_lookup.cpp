#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::binding
{
    /*
    ** Фильтр выражения пересчитывается сам, когда меняются его переменные ($query), — через
    ** ExpressionBindings::update перед кадром. Пустое выражение снимает фильтр.
    **
    ** An expression filter is recomputed by itself when its variables ($query) change — through
    ** ExpressionBindings::update before a frame. An empty expression drops the filter.
    */
    void ListBindings::setFilterExpression(const std::string& view, const std::string& expression)
    {
        data::SortFilterProxy* rows = proxy(view);
        data::TreeList* nodes = treeList(view);
        std::shared_ptr<ExpressionFilter> filter;

        if (rows == nullptr && nodes == nullptr)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_UNKNOWN_MODEL, {view}));
        if (expression.find_first_not_of(" \t\r\n") != std::string::npos)
        {
            filter = _expressions->makeFilter(expression);
            filter->setTarget(rows, nodes);
        }
        if (rows != nullptr)
            rows->setFilter(filter);
        else
            nodes->setFilter(filter);
    }

    data::SortFilterProxy* ListBindings::proxy(std::string_view name) const noexcept
    {
        return dynamic_cast<data::SortFilterProxy*>(list(name));
    }

    data::TreeList* ListBindings::treeList(std::string_view name) const noexcept
    {
        return dynamic_cast<data::TreeList*>(list(name));
    }

    ListBinding* ListBindings::bindingOf(const ui::Node& container) const noexcept
    {
        for (const std::unique_ptr<ListBinding>& binding : _bindings)
        {
            if (&binding->container() == &container)
                return binding.get();
        }
        return nullptr;
    }
}
