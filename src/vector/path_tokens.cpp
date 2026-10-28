#include "gran/vector/internal/vector_internal.hpp"

namespace gran::vector::internal
{
    // Флаг — ровно один символ 0 или 1 / A flag is exactly one character 0 or 1
    bool readFlag(PathScanner& scanner, bool& value) noexcept
    {
        skipSeparators(scanner);
        if (scanner.pos >= scanner.text.size() || (scanner.text[scanner.pos] != '0' && scanner.text[scanner.pos] != '1'))
            return false;
        value = scanner.text[scanner.pos] == '1';
        ++scanner.pos;
        return true;
    }

    // Начало числа: цифра, знак или точка / The start of a number: a digit, a sign or a dot
    bool numberAhead(PathScanner& scanner) noexcept
    {
        char symbol = 0;

        skipSeparators(scanner);
        if (scanner.pos >= scanner.text.size())
            return false;
        symbol = scanner.text[scanner.pos];
        return (symbol >= '0' && symbol <= '9') || symbol == '+' || symbol == '-' || symbol == '.';
    }

    // Нет числа там, где оно обязано быть, — ошибка в этой позиции / No number where it must be — an error at this position
    float requireNumber(PathScanner& scanner)
    {
        float value = 0.0f;

        if (!readNumber(scanner, value))
            throwPathError(scanner);
        return value;
    }
}
