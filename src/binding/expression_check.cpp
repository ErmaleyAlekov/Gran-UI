#include "gran/binding/expression_text.hpp"
#include "gran/binding/internal/expression_binding.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    namespace
    {
        // Без пробельных символов по краям / Without whitespace at the edges
        std::string_view trimmed(std::string_view text) noexcept
        {
            const char* SPACES = " \t\r\n";
            std::size_t first = text.find_first_not_of(SPACES);

            if (first == std::string_view::npos)
                return std::string_view();
            std::size_t last = text.find_last_not_of(SPACES);
            return text.substr(first, last - first + 1);
        }
    }

    // «;» не встречается в XPath 1.0 вне строк, поэтому разделитель однозначен / ";" does not occur in XPath 1.0 outside strings, so the separator is unambiguous
    void splitConverter(std::string_view part, std::string& expression, std::string& converter)
    {
        char quote = 0;

        for (std::size_t index = 0; index < part.size(); ++index)
        {
            char symbol = part[index];
            if (quote != 0 && symbol == quote)
                quote = 0;
            else if (quote == 0 && (symbol == '\'' || symbol == '"'))
                quote = symbol;
            else if (quote == 0 && symbol == ';')
            {
                expression = std::string(trimmed(part.substr(0, index)));
                converter = std::string(trimmed(part.substr(index + 1)));
                return;
            }
        }
        expression = std::string(trimmed(part));
        converter.clear();
    }

    // {name} — поле записи: в XPath это атрибут @name элемента записи / {name} is a record field: in XPath it is the @name attribute of the record element
    std::string xpathSource(std::string_view expression)
    {
        if (isFieldName(expression))
            return "@" + std::string(expression);
        return std::string(expression);
    }

    /*
    ** Ошибка синтаксиса находится при загрузке, со строкой и столбцом элемента, а не молча при первом
    ** кадре. Имя преобразователя здесь не проверяется: преобразователи регистрирует код после загрузки.
    **
    ** A syntax error is found at load time, with the element line and column, and not silently at the first
    ** frame. The converter name is not checked here: converters are registered by code after loading.
    */
    void checkExpressionText(std::string_view value)
    {
        for (const TextPart& part : splitText(value))
        {
            if (!part.expression)
                continue;
            std::string expression;
            std::string converter;
            splitConverter(part.text, expression, converter);
            try
            {
                beresta::xpath::XPathExpression compiled(xpathSource(expression));
            }
            catch (const beresta::DomException& error)
            {
                throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::BINDING_BAD_EXPRESSION,
                    {part.text, error.getMessage()}));
            }
        }
    }
}
