#include "gran/markup/internal/components.hpp"
#include "gran/binding/expression_text.hpp"

#include <string>
#include <vector>

namespace gran::markup::internal
{
    namespace
    {
        // Строковый литерал XPath 1.0: в нём нет экранирования, обе кавычки сразу — через concat / An XPath 1.0 string literal: it has no escaping, both quotes at once go through concat
        std::string quoteXPath(std::string_view text)
        {
            std::string result = "concat('";

            if (text.find('\'') == std::string_view::npos)
                return "'" + std::string(text) + "'";
            if (text.find('"') == std::string_view::npos)
                return "\"" + std::string(text) + "\"";
            for (char symbol : text)
            {
                if (symbol == '\'')
                    result += "', \"'\", '";
                else
                    result += symbol;
            }
            return result + "')";
        }

        // Выражение значения в скобках; {поле} — поле записи @поле, как у привязок; преобразователь не переносится / A value expression in brackets; {field} is the record field @field, as for the bindings; a converter does not move over
        std::string expressionPart(const std::string& part)
        {
            std::string expression;
            std::string converter;

            binding::splitConverter(part, expression, converter);
            if (binding::isFieldName(expression))
                return "@" + expression;
            return "(" + expression + ")";
        }

        // Литерал по типу параметра; $токен темы — его значение / A literal by the parameter type; a $theme token is its value
        std::string literalExpression(const ComponentParam& param, std::string text, const Substitution& substitution)
        {
            if (text.size() > 1 && text.front() == '$' && substitution.tokens->count(text.substr(1)) != 0)
                text = substitution.tokens->find(text.substr(1))->second;
            if (param.type == ParamType::NUMBER && text.empty())
                return "0";
            if (param.type == ParamType::NUMBER)
                return text;
            if (param.type != ParamType::BOOLEAN)
                return quoteXPath(text);
            if (text == "true" || text == "1")
                return "true()";
            return "false()";
        }
    }

    /*
    ** Значение экземпляра — литерал, выражение или текст вперемешку с выражениями (тогда concat):
    ** выражение экземпляра встраивается в выражение тела, и привязка отслеживает его зависимости как свои.
    ** Незаданный параметр — пустое значение своего типа.
    **
    ** The instance value is a literal, an expression or a text mixed with expressions (then concat):
    ** the instance expression is embedded into the body expression, and the binding tracks its dependencies as its own.
    ** An unset parameter is the empty value of its type.
    */
    std::string parameterExpression(const ComponentParam& param, const Substitution& substitution)
    {
        auto found = substitution.values.find(param.name);
        std::vector<std::string> parts;

        if (found == substitution.values.end())
            return literalExpression(param, std::string(), substitution);
        for (const binding::TextPart& part : binding::splitText(found->second))
        {
            if (part.expression)
                parts.push_back(expressionPart(part.text));
            else if (!part.text.empty())
                parts.push_back(quoteXPath(part.text));
        }
        if (parts.size() == 1 && found->second.find('{') == std::string::npos)
            return literalExpression(param, found->second, substitution);
        if (parts.size() == 1)
            return parts.front();
        if (parts.empty())
            return literalExpression(param, std::string(), substitution);
        std::string joined = "concat(" + parts.front();
        for (std::size_t index = 1; index < parts.size(); ++index)
            joined += ", " + parts[index];
        return joined + ")";
    }

    const ComponentParam* wholeParameter(std::string_view expression, const Substitution& substitution) noexcept
    {
        std::size_t first = expression.find_first_not_of(" \t\r\n");
        std::size_t last = expression.find_last_not_of(" \t\r\n");

        if (first == std::string_view::npos || expression[first] != '$')
            return nullptr;
        std::string_view name = expression.substr(first + 1, last - first);
        for (const ComponentParam& param : substitution.spec->params)
        {
            if (name == param.name)
                return &param;
        }
        return nullptr;
    }
}
