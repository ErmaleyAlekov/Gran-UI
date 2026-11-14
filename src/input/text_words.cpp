#include "gran/input/internal/text_cursor.hpp"
#include "gran/utils/utf8.hpp"

namespace gran::input::internal
{
    namespace
    {
        // Классы символов для двойного щелчка / The character classes for a double click
        enum CharClass { CLASS_SPACE, CLASS_WORD, CLASS_MARK, CLASS_NEWLINE };

        /*
        ** Класс кода: пробелы (вкл. неразрывный и идеографический), перевод строки, слово
        ** (буквы и цифры ASCII, '_', любые символы вне ASCII, кроме знаков препинания Latin-1,
        ** общей пунктуации U+2000–U+206F и пунктуации CJK U+3000–U+303F), остальное — знаки.
        **
        ** The code class: spaces (incl. the no-break and ideographic ones), a line break, a word
        ** (ASCII letters and digits, '_', any non-ASCII characters except the Latin-1 punctuation,
        ** the general punctuation U+2000–U+206F and the CJK punctuation U+3000–U+303F), the rest are marks.
        */
        CharClass classOf(char32_t symbol) noexcept
        {
            if (symbol == '\n' || symbol == '\r')
                return CLASS_NEWLINE;
            if (symbol == ' ' || symbol == '\t' || symbol == 0xA0 || symbol == 0x3000)
                return CLASS_SPACE;
            if ((symbol >= '0' && symbol <= '9') || (symbol >= 'a' && symbol <= 'z')
                || (symbol >= 'A' && symbol <= 'Z') || symbol == '_')
                return CLASS_WORD;
            if (symbol < 0xC0 || (symbol >= 0x2000 && symbol <= 0x206F) || (symbol >= 0x3000 && symbol <= 0x303F))
                return CLASS_MARK;
            return CLASS_WORD;
        }

        // Класс символа, начинающегося с байта position / The class of the character starting at the byte position
        CharClass classAt(const std::string& text, std::size_t position) noexcept
        {
            return classOf(decodeUtf8(text, position));
        }
    }

    // Расширение влево и вправо, пока класс тот же; position в конце текста — последний символ
    // Extending left and right while the class is the same; position at the text end is the last character
    void wordBounds(const std::string& text, std::size_t position, std::size_t& from, std::size_t& to)
    {
        from = 0;
        to = 0;
        if (text.empty())
            return;
        if (position >= text.size())
            position = previousBoundary(text, text.size());
        CharClass kind = classAt(text, position);
        from = position;
        while (from > 0 && classAt(text, previousBoundary(text, from)) == kind)
            from = previousBoundary(text, from);
        to = nextBoundary(text, position);
        while (to < text.size() && classAt(text, to) == kind)
            to = nextBoundary(text, to);
    }

    // Пробелы — промежутки шагов по словам; перевод строки — нет (isLineBreak) / Spaces are the word step gaps; a line break is not (isLineBreak)
    bool isWordGap(const std::string& text, std::size_t position) noexcept
    {
        return position < text.size() && classAt(text, position) == CLASS_SPACE;
    }

    // Перевод строки останавливает шаг по слову (замечание Т-46: пустые строки проскакивались насквозь)
    // A line break stops a word step (remark T-46: empty lines were skipped through)
    bool isLineBreak(const std::string& text, std::size_t position) noexcept
    {
        return position < text.size() && classAt(text, position) == CLASS_NEWLINE;
    }
}
