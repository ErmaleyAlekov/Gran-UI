#include "gran/style/value_scanner.hpp"

namespace gran::style
{
    namespace
    {
        // Буква ASCII или '_' / An ASCII letter or '_'
        bool isLetter(char symbol) noexcept
        {
            return (symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z') || symbol == '_';
        }

        // Символ, допустимый внутри идентификатора / A character allowed inside an identifier
        bool isIdentifierPart(char symbol) noexcept
        {
            return isLetter(symbol) || symbol == '-' || (symbol >= '0' && symbol <= '9');
        }
    }

    // Идентификатор: первый символ — буква, '_' или '-' перед буквой
    // An identifier: the first character is a letter, '_' or '-' before a letter
    std::string_view ValueScanner::readIdentifier() noexcept
    {
        atEnd();
        std::size_t start = _position;
        std::size_t position = start;

        if (position < _text.size() && _text[position] == '-')
            ++position;
        if (position >= _text.size() || !isLetter(_text[position]))
            return std::string_view();
        while (position < _text.size() && isIdentifierPart(_text[position]))
            ++position;
        _position = position;
        return _text.substr(start, position - start);
    }

    // После '(' — всё до парной ')', затем разбиение по запятым верхнего уровня
    // After '(' — everything up to the matching ')', then a split by top level commas
    bool ValueScanner::readArguments(std::vector<std::string_view>& arguments) noexcept
    {
        int depth = 1;

        if (!consume('('))
            return false;
        std::size_t start = _position;
        while (_position < _text.size() && depth > 0)
        {
            if (_text[_position] == '(')
                ++depth;
            else if (_text[_position] == ')')
                --depth;
            ++_position;
        }
        if (depth != 0)
            return false;
        arguments = splitTopLevel(_text.substr(start, _position - 1 - start), ',');
        return true;
    }

    // Остаток текста / The rest of the text
    std::string_view ValueScanner::rest() const noexcept
    {
        return _text.substr(_position);
    }
}
