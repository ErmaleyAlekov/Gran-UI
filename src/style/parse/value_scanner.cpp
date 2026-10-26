#include "gran/style/value_scanner.hpp"

namespace gran::style
{
    namespace
    {
        // Пробельные символы XML / The XML whitespace characters
        bool isSpace(char symbol) noexcept
        {
            return symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r';
        }
    }

    // Сканер начинает с начала текста / The scanner starts at the beginning of the text
    ValueScanner::ValueScanner(std::string_view text) noexcept
        : _text(text), _position(0)
    {
    }

    // Пробелы пропускаются, затем проверяется конец
    // The spaces are skipped, then the end is checked
    bool ValueScanner::atEnd() noexcept
    {
        while (_position < _text.size() && isSpace(_text[_position]))
            ++_position;
        return _position >= _text.size();
    }

    // Следующий значимый символ / The next significant character
    char ValueScanner::peek() noexcept
    {
        if (atEnd())
            return '\0';
        return _text[_position];
    }

    // Символ съедается, только если совпал / The character is eaten only if it matches
    bool ValueScanner::consume(char expected) noexcept
    {
        if (peek() != expected)
            return false;
        ++_position;
        return true;
    }
}
