#include "gran/text/internal/text_internal.hpp"

namespace gran::text
{
    namespace
    {
        // Иероглифы и слоги CJK, полноширинные формы / CJK ideographs and syllables, fullwidth forms
        bool isCjk(char32_t symbol) noexcept
        {
            return (symbol >= 0x2E80 && symbol <= 0x9FFF) || (symbol >= 0xAC00 && symbol <= 0xD7AF)
                || (symbol >= 0xF900 && symbol <= 0xFAFF) || (symbol >= 0xFF00 && symbol <= 0xFFEF)
                || (symbol >= 0x20000 && symbol <= 0x2FFFF);
        }

        // Буква для правила дефиса: латиница и всё выше U+00BF (кириллица и т. д.)
        // A letter for the hyphen rule: Latin and everything above U+00BF (Cyrillic and so on)
        bool isLetter(char32_t symbol) noexcept
        {
            return (symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z') || symbol >= 0x00C0;
        }
    }

    // Пробелы, по которым можно переносить и растягивать строку (U+00A0 сюда не входит)
    // The spaces at which a line may break and stretch (U+00A0 is not one of them)
    bool isBreakingSpace(char32_t symbol) noexcept
    {
        return symbol == ' ' || symbol == '\t' || symbol == 0x3000;
    }

    // Правила переноса из заголовка / The break rules from the header
    bool breakOpportunityAfter(char32_t symbol, char32_t next) noexcept
    {
        if (isBreakingSpace(symbol) || symbol == 0x200B)
            return true;
        if ((symbol == '-' || symbol == 0x2010) && isLetter(next))
            return true;
        return isCjk(symbol) || isCjk(next);
    }
}
