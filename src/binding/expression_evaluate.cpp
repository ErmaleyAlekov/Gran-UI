#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    namespace
    {
        using beresta::xpath::XPathValue;

        /*
        ** Узел контекста: элемент записи в зеркале (у копии шаблона списка), иначе корень <data>.
        ** $index — номер записи (вне списка — 0); модель записи запоминается как зависимость.
        **
        ** The context node: the record element in the mirror (for a list template copy), otherwise the <data> root.
        ** $index is the record number (outside a list — 0); the record model is remembered as a dependency.
        */
        const beresta::Node& contextNode(EvaluationScope& scope, ExpressionBinding& binding,
            beresta::xpath::XPathContext& context)
        {
            ItemRef item = scope.owner.itemOf(*binding.node);

            binding.item = item.model;
            context.variables.insert_or_assign("index", XPathValue::fromNumber(static_cast<double>(item.index)));
            if (item.model == nullptr)
                return scope.mirror.root();
            binding.itemSeen = scope.mirror.version(*item.model);
            const beresta::Element* record = scope.mirror.record(*item.model, item.index);
            if (record == nullptr)
                return scope.mirror.root();
            return *record;
        }

        // Значения переменных выражения; заодно запоминается то, с чем сравнит dependenciesChanged / The expression variable values; what dependenciesChanged compares with is remembered at the same time
        void fillVariables(EvaluationScope& scope, ExpressionBinding& binding, beresta::xpath::XPathContext& context)
        {
            for (PropertyWatch& watch : binding.properties)
            {
                XPathValue value = propertyValue(propertyNode(scope, binding, watch.base), watch.property);
                watch.seen = value.toString();
                context.variables.insert_or_assign(watch.variable, std::move(value));
            }
            for (VersionWatch& watch : binding.variables)
            {
                watch.seen = scope.bindings.variableVersion(watch.name);
                context.variables.insert_or_assign(watch.name, variableValue(scope.bindings.variable(watch.name)));
            }
            for (VersionWatch& watch : binding.models)
                watch.seen = scope.mirror.version(watch.name);
        }

        /*
        ** Текст значения. У логического атрибута множество элементов значит «есть ли хоть один»
        ** (visible="{files/Item}"), а атрибут записи — его текст: checked="{done}" при done="false" ложно.
        **
        ** The value text. For a boolean attribute a set of elements means "whether there is at least one"
        ** (visible="{files/Item}"), while a record attribute means its text: checked="{done}" with done="false" is false.
        */
        std::string valueText(const XPathValue& value, bool boolean)
        {
            if (!boolean || value.getType() != beresta::xpath::ValueType::NODE_SET)
                return value.toString();
            if (!value.getNodes().empty() && value.getNodes().front().attribute != nullptr)
                return value.toString();
            return XPathValue::fromBoolean(value.toBoolean()).toString();
        }
    }

    bool dependenciesChanged(EvaluationScope& scope, ExpressionBinding& binding)
    {
        for (const PropertyWatch& watch : binding.properties)
        {
            if (propertyValue(propertyNode(scope, binding, watch.base), watch.property).toString() != watch.seen)
                return true;
        }
        for (const VersionWatch& watch : binding.variables)
        {
            if (scope.bindings.variableVersion(watch.name) != watch.seen)
                return true;
        }
        for (const VersionWatch& watch : binding.models)
        {
            if (scope.mirror.version(watch.name) != watch.seen)
                return true;
        }
        return binding.item != nullptr && scope.mirror.version(*binding.item) != binding.itemSeen;
    }

    // Части склеиваются: литералы как есть, выражения — через свой преобразователь / The parts are joined: the literals as they are, the expressions — through their converter
    std::string evaluateBinding(EvaluationScope& scope, ExpressionBinding& binding)
    {
        beresta::xpath::XPathContext context;
        const beresta::Node& node = contextNode(scope, binding, context);
        bool boolean = booleanTarget(binding.attribute);
        std::string result;

        fillVariables(scope, binding, context);
        for (const CompiledPart& part : binding.parts)
        {
            if (!part.expression)
            {
                result += part.literal;
                continue;
            }
            std::string text = valueText(part.expression->evaluate(node, context), boolean);
            result += scope.bindings.toView(part.converter, text);
        }
        return result;
    }
}
