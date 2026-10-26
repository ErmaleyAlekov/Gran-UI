#ifndef GRAN_STYLE_VALUE_SCANNER_HPP
#define GRAN_STYLE_VALUE_SCANNER_HPP

#include <cstddef>
#include <string_view>
#include <vector>

namespace gran::style
{
    /*
    ** Сканер текста значения стиля: числа, идентификаторы, символы.
    ** Не выделяет память и не зависит от локали (точка — всегда десятичный
    ** разделитель, в отличие от strtof). Пробелы перед лексемой пропускаются.
    **
    ** A style value text scanner: numbers, identifiers, characters.
    ** It does not allocate memory and does not depend on the locale (a dot is always
    ** the decimal separator, unlike strtof). Spaces before a token are skipped.
    */
    class ValueScanner
    {
    public:
        explicit ValueScanner(std::string_view text) noexcept;

        // Пропустить пробелы; true — текст кончился / Skip spaces; true — the text has ended
        bool atEnd() noexcept;

        // Следующий символ без сдвига ('\0' в конце) / The next character without moving ('\0' at the end)
        char peek() noexcept;

        // Съесть символ, если он следующий / Eat the character if it is next
        bool consume(char expected) noexcept;

        // Число: [+-]цифры[.цифры][e[+-]цифры]; false — числа нет, позиция прежняя
        // A number: [+-]digits[.digits][e[+-]digits]; false — no number, the position stays
        bool readNumber(float& value) noexcept;

        // Идентификатор: буквы, цифры, '-', '_' (не с цифры); пусто — его нет
        // An identifier: letters, digits, '-', '_' (not from a digit); empty — there is none
        std::string_view readIdentifier() noexcept;

        // Аргументы функции до парной ')' с разбиением по запятым верхнего уровня
        // The function arguments up to the matching ')' split by top level commas
        bool readArguments(std::vector<std::string_view>& arguments) noexcept;

        // Остаток текста от текущей позиции / The rest of the text from the current position
        std::string_view rest() const noexcept;

    private:
        // Цифры подряд; возвращает их число / Digits in a row; returns their count
        std::size_t readDigits(double& accumulator, double& scale, bool fraction) noexcept;

        // Показатель степени "e[+-]цифры" или 0, если его нет
        // The exponent "e[+-]digits" or 0 if there is none
        double readExponent() noexcept;

        std::string_view _text;
        std::size_t _position;
    };

    // Срез без пробелов по краям / The slice without spaces at the edges
    std::string_view trimSpaces(std::string_view text) noexcept;

    // Разбиение по разделителю верхнего уровня (вне скобок), части без пробелов по краям
    // Splitting by a top level separator (outside parentheses), the parts without edge spaces
    std::vector<std::string_view> splitTopLevel(std::string_view text, char separator);

    // Части через пробелы вне скобок, без пустых: "a  b(1 2)" → ["a", "b(1 2)"]
    // Parts separated by spaces outside parentheses, without empty ones: "a  b(1 2)" → ["a", "b(1 2)"]
    std::vector<std::string_view> splitSpaces(std::string_view text);
}

#endif
