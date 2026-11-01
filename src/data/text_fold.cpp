#include "gran/data/record_order.hpp"
#include "gran/utils/utf8.hpp"

namespace gran::data
{
    namespace
    {
        /*
        ** Своя свёртка регистра для письменностей, которые движок поддерживает: ASCII, Latin-1,
        ** греческий и кириллица (Ё и прочие буквы Ѐ–Џ). Полная таблица Unicode здесь не нужна:
        ** сравнение служит сортировке, а не лингвистике.
        **
        ** An own case folding for the scripts the engine supports: ASCII, Latin-1,
        ** Greek and Cyrillic (Ё and the other letters Ѐ–Џ). The full Unicode table is not needed here:
        ** the comparison serves sorting, not linguistics.
        */
        char32_t foldSymbol(char32_t symbol) noexcept
        {
            if (symbol >= U'A' && symbol <= U'Z')
                return symbol + 0x20;
            if (symbol >= 0xC0 && symbol <= 0xDE && symbol != 0xD7)
                return symbol + 0x20;
            if (symbol >= 0x391 && symbol <= 0x3A9 && symbol != 0x3A2)
                return symbol + 0x20;
            if (symbol >= 0x410 && symbol <= 0x42F)
                return symbol + 0x20;
            if (symbol >= 0x400 && symbol <= 0x40F)
                return symbol + 0x50;
            return symbol;
        }
    }

    std::string foldCase(std::string_view text)
    {
        std::string result;
        std::size_t position = 0;

        result.reserve(text.size());
        while (position < text.size())
            appendUtf8(result, foldSymbol(decodeUtf8(text, position)));
        return result;
    }
}
