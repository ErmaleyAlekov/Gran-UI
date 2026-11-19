#include "gran/data/cell_value.hpp"
#include "gran/data/internal/cell_formats.hpp"

namespace gran::data
{
    namespace
    {
        // Текст без пробельных символов ASCII по краям / The text without the ASCII whitespace at the edges
        std::string_view trimmed(std::string_view text) noexcept
        {
            const std::string_view SPACES = " \t\r\n";
            std::size_t first = text.find_first_not_of(SPACES);

            if (first == std::string_view::npos)
                return std::string_view();
            std::size_t last = text.find_last_not_of(SPACES);
            return text.substr(first, last - first + 1);
        }

        // Совпадение без учёта регистра латиницы (word — в нижнем регистре) / A match ignoring the Latin case (word is lower-case)
        bool sameWord(std::string_view text, std::string_view word) noexcept
        {
            if (text.size() != word.size())
                return false;
            for (std::size_t index = 0; index < text.size(); ++index)
            {
                char letter = text[index];
                if (letter >= 'A' && letter <= 'Z')
                    letter = static_cast<char>(letter - 'A' + 'a');
                if (letter != word[index])
                    return false;
            }
            return true;
        }

        /*
        ** Логическое слово → 1 (истина), 0 (ложь), -1 (не логическое). «да» и «нет» — в нижнем регистре:
        ** свой перевод регистра кириллицы ради двух слов не нужен.
        **
        ** A boolean word → 1 (true), 0 (false), -1 (not boolean). "да" and "нет" are lower-case:
        ** an own Cyrillic case conversion is not needed for two words.
        */
        int booleanOf(std::string_view text) noexcept
        {
            static const char* const YES[] = {"true", "1", "yes", "on", "\xD0\xB4\xD0\xB0"};
            static const char* const NO[] = {"false", "0", "no", "off", "\xD0\xBD\xD0\xB5\xD1\x82"};

            for (const char* word : YES)
            {
                if (sameWord(text, word))
                    return 1;
            }
            for (const char* word : NO)
            {
                if (sameWord(text, word))
                    return 0;
            }
            return -1;
        }
    }

    bool normalizeValue(ColumnType type, std::string& text)
    {
        std::string_view value = trimmed(text);
        std::string result;
        int flag = -1;

        if (type == ColumnType::TEXT)
            return true;
        // Логическое слово ищется один раз (ревью Т-69) / The boolean word is looked up once (review T-69)
        if (type == ColumnType::BOOLEAN)
            flag = booleanOf(value);
        if (value.empty())
        {
            text.clear();
            return true;
        }
        if (type == ColumnType::NUMBER && !internal::normalizeNumber(value, result))
            return false;
        if (type == ColumnType::DATE && !internal::normalizeDate(value, result))
            return false;
        if (type == ColumnType::BOOLEAN && flag < 0)
            return false;
        if (type == ColumnType::BOOLEAN && flag == 1)
            result = "true";
        else if (type == ColumnType::BOOLEAN)
            result = "false";
        text = std::move(result);
        return true;
    }

    bool valueTrue(std::string_view text)
    {
        return booleanOf(trimmed(text)) == 1;
    }
}
