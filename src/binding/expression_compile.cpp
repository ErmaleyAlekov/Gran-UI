#include "gran/binding/internal/expression_binding.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    namespace
    {
        // Выражение части: XPath, его зависимости и преобразователь / The part expression: XPath, its dependencies and the converter
        CompiledPart compilePart(const TextPart& part, ExpressionBinding& binding)
        {
            CompiledPart compiled;
            std::string expression;

            if (!part.expression)
            {
                compiled.literal = part.text;
                return compiled;
            }
            splitConverter(part.text, expression, compiled.converter);
            std::string source = xpathSource(expression);
            scanDependencies(source, binding);
            try
            {
                compiled.expression = std::make_shared<const beresta::xpath::XPathExpression>(source);
            }
            catch (const beresta::DomException& error)
            {
                throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::BINDING_BAD_EXPRESSION,
                    {part.text, error.getMessage()}));
            }
            return compiled;
        }

        // Ровно одно выражение без литералов вокруг / Exactly one expression without literals around
        const CompiledPart* singleExpression(const ExpressionBinding& binding) noexcept
        {
            const CompiledPart* found = nullptr;

            for (const CompiledPart& part : binding.parts)
            {
                if (!part.expression && part.literal.empty())
                    continue;
                if (!part.expression || found != nullptr)
                    return nullptr;
                found = &part;
            }
            return found;
        }

        /*
        ** Двусторонняя привязка — когда выражение само есть место для записи: $имя (не свойство узла
        ** и не $index) или @поле записи, а атрибут меняет пользователь (текст поля, отметка, value).
        **
        ** A two-way binding is when the expression itself is a place to write to: $name (not a node property
        ** and not $index) or a record @field, and the user changes the attribute (the field text, the mark, value).
        */
        void detectSource(ExpressionBinding& binding)
        {
            const CompiledPart* part = singleExpression(binding);

            if (part == nullptr || !editableTarget(*binding.node, binding.attribute))
                return;
            const std::string& source = part->expression->getSource();
            if (source.size() < 2)
                return;
            std::string_view name = std::string_view(source).substr(1);
            if (!isFieldName(name))
                return;
            if (source[0] == '@')
                binding.source = SourceKind::FIELD;
            else if (source[0] == '$' && name.find('.') == std::string_view::npos && name != "index"
                && !rowVariable(name))
                binding.source = SourceKind::VARIABLE;
            binding.sourceName = std::string(name);
        }
    }

    std::unique_ptr<ExpressionBinding> compileBinding(ui::Node& node, const ui::AttributeBinding& source)
    {
        std::unique_ptr<ExpressionBinding> binding = std::make_unique<ExpressionBinding>();

        binding->node = &node;
        binding->attribute = source.attribute;
        for (const TextPart& part : splitText(source.source))
            binding->parts.push_back(compilePart(part, *binding));
        detectSource(*binding);
        return binding;
    }
}
