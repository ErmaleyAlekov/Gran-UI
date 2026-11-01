#include "gran/binding/internal/expression_binding.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    /*
    ** Зависимости собираются тем же сканером, что и у привязок; в фильтре важны только переменные
    ** приложения ($query): свойства узлов и модели здесь не отслеживаются — у фильтра нет узла.
    **
    ** The dependencies are collected by the same scanner as for bindings; only the application variables
    ** matter in a filter ($query): node properties and models are not tracked here — a filter has no node.
    */
    ExpressionFilter::ExpressionFilter(const ExpressionBindings& bindings, const std::string& expression)
        : _bindings(&bindings)
    {
        ExpressionBinding dependencies;
        std::string source = xpathSource(expression);

        scanDependencies(source, dependencies);
        _variables = std::move(dependencies.variables);
        try
        {
            _expression = std::make_shared<const beresta::xpath::XPathExpression>(source);
        }
        catch (const beresta::DomException& error)
        {
            throw GranException(ErrorCode::INVALID_ARGUMENT,
                i18n::format(i18n::Message::BINDING_BAD_EXPRESSION, {expression, error.getMessage()}));
        }
        for (VersionWatch& watch : _variables)
            watch.seen = bindings.variableVersion(watch.name);
    }

    ExpressionFilter::~ExpressionFilter() = default;

    bool ExpressionFilter::accepts(const data::ListSource& source, std::size_t index) const
    {
        return evaluate(source.at(index));
    }

    bool ExpressionFilter::accepts(const data::TreeModel& tree, data::NodeId node) const
    {
        return evaluate(tree.record(node));
    }

    void ExpressionFilter::setTarget(data::SortFilterProxy* proxy, data::TreeList* tree) noexcept
    {
        _proxy = proxy;
        _tree = tree;
    }
}
