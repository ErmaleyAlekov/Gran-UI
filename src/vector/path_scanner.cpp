#include "gran/vector/internal/vector_internal.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "gran/style/value_parsers.hpp"

namespace gran::vector::internal
{
    namespace
    {
        // Пробельные символы XML / The XML whitespace characters
        bool isSpace(char symbol) noexcept
        {
            return symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r' || symbol == '\f';
        }

        // Цифры подряд с позиции; возвращает их число / The digits in a row from the position; returns their count
        std::size_t skipDigits(std::string_view text, std::size_t& pos) noexcept
        {
            std::size_t begin = pos;

            while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9')
                ++pos;
            return pos - begin;
        }
    }

    // Пробелы, затем не больше одной запятой и снова пробелы / Spaces, then at most one comma and spaces again
    void skipSeparators(PathScanner& scanner) noexcept
    {
        while (scanner.pos < scanner.text.size() && isSpace(scanner.text[scanner.pos]))
            ++scanner.pos;
        if (scanner.pos >= scanner.text.size() || scanner.text[scanner.pos] != ',')
            return;
        ++scanner.pos;
        while (scanner.pos < scanner.text.size() && isSpace(scanner.text[scanner.pos]))
            ++scanner.pos;
    }

    /*
    ** Число SVG: знак, цифры, точка, цифры, экспонента. Число кончается там, где грамматика
    ** не может его продолжить, поэтому «1.5.5» — это 1.5 и .5, а «1-2» — 1 и −2.
    ** Перевод — style::parseNumber (не зависит от локали: «,» никогда не десятичный знак; std::from_chars
    ** для float нет в libstdc++ до GCC 11 — Astra Linux SE 1.7, ALT p10).
    **
    ** An SVG number: a sign, digits, a dot, digits, an exponent. A number ends where the grammar
    ** cannot continue it, so "1.5.5" is 1.5 and .5, and "1-2" is 1 and −2.
    ** The conversion is style::parseNumber (locale independent: "," is never a decimal mark; std::from_chars
    ** for float is missing in libstdc++ before GCC 11 — Astra Linux SE 1.7, ALT p10).
    */
    bool readNumber(PathScanner& scanner, float& value)
    {
        std::string_view text = scanner.text;
        std::size_t pos = 0;
        std::size_t digits = 0;

        skipSeparators(scanner);
        pos = scanner.pos;
        if (pos < text.size() && (text[pos] == '+' || text[pos] == '-'))
            ++pos;
        digits = skipDigits(text, pos);
        if (pos < text.size() && text[pos] == '.')
            digits += skipDigits(text, ++pos);
        if (digits == 0)
            return false;
        std::size_t mantissaEnd = pos;
        if (pos < text.size() && (text[pos] == 'e' || text[pos] == 'E'))
        {
            ++pos;
            if (pos < text.size() && (text[pos] == '+' || text[pos] == '-'))
                ++pos;
            if (skipDigits(text, pos) == 0)
                pos = mantissaEnd;
        }
        // Кусок уже проверен грамматикой выше / The piece is already checked by the grammar above
        value = style::parseNumber(text.substr(scanner.pos, pos - scanner.pos));
        scanner.pos = pos;
        return true;
    }

    // Позиция — номер символа с нуля, как его видит автор разметки / The position is the character number from zero, as the markup author sees it
    void throwPathError(const PathScanner& scanner)
    {
        throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::ICON_BAD_PATH, {std::to_string(scanner.pos)}));
    }
}
