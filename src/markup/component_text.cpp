#include "gran/markup/internal/components.hpp"
#include "gran/binding/expression_text.hpp"

#include <string>

namespace gran::markup::internal
{
    namespace
    {
        // Конец имени после $: буквы, цифры, _ - . и байты UTF-8, как у лексера XPath «Берёсты» / The end of a name after $: letters, digits, _ - . and UTF-8 bytes, as for the Beresta XPath lexer
        std::size_t nameEnd(std::string_view text, std::size_t position) noexcept
        {
            while (position < text.size())
            {
                unsigned char symbol = static_cast<unsigned char>(text[position]);
                bool part = (symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z') || symbol == '_'
                    || symbol >= 0x80 || (symbol >= '0' && symbol <= '9') || symbol == '-' || symbol == '.';
                if (!part)
                    break;
                ++position;
            }
            return position;
        }

        /*
        ** $имя: параметр компонента → его значение XPath; $локальный.свойство (точка — последняя, как у
        ** привязок) → $полный-id.свойство; прочее (переменные окна, $parent.width) — как было.
        **
        ** $name: a component parameter → its XPath value; $local.property (the dot is the last one, as for
        ** the bindings) → $full-id.property; the rest (window variables, $parent.width) — as it was.
        */
        std::string replaceName(std::string_view name, const Substitution& substitution)
        {
            std::size_t dot = name.rfind('.');

            for (const ComponentParam& param : substitution.spec->params)
            {
                if (name == param.name)
                    return parameterExpression(param, substitution);
            }
            if (dot != std::string_view::npos)
            {
                auto found = substitution.ids.find(name.substr(0, dot));
                if (found != substitution.ids.end())
                    return "$" + found->second + std::string(name.substr(dot));
            }
            return "$" + std::string(name);
        }

        // Имена $… вне строковых литералов XPath; незакрытая строка копируется до конца / The $… names outside the XPath string literals; an unclosed string is copied up to the end
        std::string substituteExpression(std::string_view expression, const Substitution& substitution)
        {
            std::string result;
            std::size_t position = 0;

            while (position < expression.size())
            {
                char symbol = expression[position];
                std::size_t end = position + 1;
                if (symbol == '\'' || symbol == '"')
                {
                    end = expression.find(symbol, position + 1);
                    if (end == std::string_view::npos)
                        end = expression.size() - 1;
                    result.append(expression.substr(position, end + 1 - position));
                    ++end;
                }
                else if (symbol == '$')
                {
                    end = nameEnd(expression, position + 1);
                    result += replaceName(expression.substr(position + 1, end - position - 1), substitution);
                }
                else
                    result += symbol;
                position = end;
            }
            return result;
        }
    }

    std::optional<std::string> substituteAttribute(std::string_view value, const Substitution& substitution)
    {
        if (binding::isWholeExpression(value))
        {
            const ComponentParam* param = wholeParameter(value.substr(1, value.size() - 2), substitution);
            if (param != nullptr && substitution.values.find(param->name) == substitution.values.end())
                return std::nullopt;
        }
        return substituteText(value, substitution);
    }

    /*
    ** Текст переписывается как есть, кроме выражений: «{$имя}» целиком заменяется значением экземпляра
    ** (литерал, $токен темы или его собственное выражение {…}), в остальных выражениях подставляются имена.
    ** «{{» и «}}» остаются удвоенными — их раскроет привязка.
    **
    ** The text is rewritten as is except the expressions: a whole "{$name}" is replaced by the instance value
    ** (a literal, a $theme token or its own {…} expression), in the other expressions the names are substituted.
    ** "{{" and "}}" stay doubled — the binding expands them.
    */
    std::string substituteText(std::string_view value, const Substitution& substitution)
    {
        std::string result;
        std::size_t position = 0;

        while (position < value.size())
        {
            char symbol = value[position];
            std::size_t close = std::string_view::npos;
            bool doubled = (symbol == '{' || symbol == '}') && position + 1 < value.size() && value[position + 1] == symbol;
            if (symbol == '{' && !doubled)
                close = binding::findExpressionClose(value, position + 1);
            if (close == std::string_view::npos)
            {
                result.append(value.substr(position, 1 + static_cast<std::size_t>(doubled)));
                position += 1 + static_cast<std::size_t>(doubled);
                continue;
            }
            std::string_view inner = value.substr(position + 1, close - position - 1);
            const ComponentParam* param = wholeParameter(inner, substitution);
            if (param == nullptr)
                result.append("{").append(substituteExpression(inner, substitution)).append("}");
            else if (substitution.values.count(param->name) != 0)
                result += substitution.values.at(param->name);
            position = close + 1;
        }
        return result;
    }
}
