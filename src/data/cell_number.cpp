#include "gran/data/internal/cell_formats.hpp"

namespace gran::data::internal
{
    namespace
    {
        // Цифры подряд с позиции at переписываются в result; возвращается их число / The digits in a row from at are copied into result; their count is returned
        std::size_t copyDigits(std::string_view text, std::size_t& at, std::string& result)
        {
            std::size_t start = at;

            while (at < text.size() && text[at] >= '0' && text[at] <= '9')
                result += text[at++];
            return at - start;
        }

        // Порядок «e[+|-]цифры» с позиции at (буква уже на месте); false — после буквы нет цифр / The exponent "e[+|-]digits" from at (the letter is in place); false — no digits after the letter
        bool copyExponent(std::string_view text, std::size_t& at, std::string& result)
        {
            result += 'e';
            ++at;
            if (at < text.size() && (text[at] == '+' || text[at] == '-'))
            {
                if (text[at] == '-')
                    result += '-';
                ++at;
            }
            return copyDigits(text, at, result) > 0;
        }
    }

    /*
    ** Разбор своим автоматом, а не strtod: strtod зависит от локали (запятая или точка), принимает
    ** «inf», «nan», шестнадцатеричные числа и пробелы перед числом — в ячейке таблицы это не числа.
    **
    ** Parsing by an own automaton, not strtod: strtod depends on the locale (a comma or a dot), accepts
    ** "inf", "nan", hexadecimal numbers and spaces before the number — in a table cell these are not numbers.
    */
    bool normalizeNumber(std::string_view text, std::string& result)
    {
        std::size_t at = 0;
        std::size_t digits = 0;

        result.clear();
        if (at < text.size() && (text[at] == '+' || text[at] == '-'))
        {
            if (text[at] == '-')
                result += '-';
            ++at;
        }
        digits = copyDigits(text, at, result);
        if (at < text.size() && (text[at] == '.' || text[at] == ','))
        {
            result += '.';
            ++at;
            digits += copyDigits(text, at, result);
        }
        if (digits == 0)
            return false;
        if (at < text.size() && (text[at] == 'e' || text[at] == 'E') && !copyExponent(text, at, result))
            return false;
        return at == text.size();
    }
}
