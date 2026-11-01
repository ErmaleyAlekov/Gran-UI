#include "gran/binding/internal/expression_binding.hpp"

#include <algorithm>

namespace gran::binding
{
    namespace
    {
        // Первый символ имени: буква, _ или байт UTF-8 / The first name character: a letter, _ or a UTF-8 byte
        bool startsName(char symbol) noexcept
        {
            unsigned char code = static_cast<unsigned char>(symbol);

            return (code >= 'a' && code <= 'z') || (code >= 'A' && code <= 'Z') || code == '_' || code >= 0x80;
        }

        // Конец имени NCName (буквы, цифры, _ - . и байты UTF-8; «:» с префиксом — как лексер «Берёсты») / The end of an NCName (letters, digits, _ - . and UTF-8 bytes; ":" with a prefix — as the Beresta lexer)
        std::size_t nameEnd(std::string_view text, std::size_t position) noexcept
        {
            while (position < text.size())
            {
                char symbol = text[position];
                bool part = startsName(symbol) || (symbol >= '0' && symbol <= '9') || symbol == '-' || symbol == '.';
                bool prefix = symbol == ':' && position + 1 < text.size() && text[position + 1] != ':';
                if (!part && !prefix)
                    break;
                ++position;
            }
            return position;
        }

        /*
        ** $index — номер записи; $узел.свойство — свойство узла (точка — последняя: у id может быть своя);
        ** остальное — переменная приложения. Повтор той же переменной добавляет лишь безвредную повторную проверку.
        **
        ** $index is the record number; $node.property is a node property (the dot is the last one: an id may have its own);
        ** the rest is an application variable. A repeat of the same variable only adds a harmless duplicate check.
        */
        void addVariable(ExpressionBinding& binding, std::string_view name)
        {
            std::size_t dot = name.rfind('.');

            if (name.empty())
                return;
            if (name == "index")
                binding.usesIndex = true;
            else if (dot != std::string_view::npos && dot > 0 && dot + 1 < name.size())
                binding.properties.push_back(PropertyWatch{std::string(name), std::string(name.substr(0, dot)),
                    std::string(name.substr(dot + 1)), std::string()});
            else
                binding.variables.push_back(VersionWatch{std::string(name), 0});
        }

        /*
        ** Имя шага пути может быть моделью (files в count(files/Item)). Функции (имя перед «(»), оси
        ** (перед «::») и операторы — не модели. Лишнее имя безвредно: версия несуществующей модели — 0.
        **
        ** A path step name may be a model (files in count(files/Item)). Functions (a name before "("), axes
        ** (before "::") and operators are not models. An extra name is harmless: a missing model version is 0.
        */
        void addModelName(ExpressionBinding& binding, std::string_view text, std::size_t start, std::size_t end)
        {
            std::string_view name = text.substr(start, end - start);
            std::size_t next = text.find_first_not_of(" \t\r\n", end);

            if (next != std::string_view::npos && (text[next] == '(' || text.compare(next, 2, "::") == 0))
                return;
            if (name == "and" || name == "or" || name == "div" || name == "mod" || name == "Item")
                return;
            for (const VersionWatch& watch : binding.models)
            {
                if (watch.name == name)
                    return;
            }
            binding.models.push_back(VersionWatch{std::string(name), 0});
        }
    }

    // Строковые литералы пропускаются: имена внутри них — текст, а не зависимости / String literals are skipped: the names inside them are text, not dependencies
    void scanDependencies(std::string_view expression, ExpressionBinding& binding)
    {
        std::size_t position = 0;

        while (position < expression.size())
        {
            char symbol = expression[position];
            std::size_t end = position + 1;
            if (symbol == '\'' || symbol == '"')
            {
                end = expression.find(symbol, position + 1);
                if (end == std::string_view::npos)
                    return;
                ++end;
            }
            else if (symbol == '$')
            {
                end = nameEnd(expression, position + 1);
                addVariable(binding, expression.substr(position + 1, end - position - 1));
            }
            else if (symbol == '@')
                end = nameEnd(expression, position + 1);
            else if (startsName(symbol))
            {
                end = nameEnd(expression, position);
                addModelName(binding, expression, position, end);
            }
            position = std::max(end, position + 1);
        }
    }
}
