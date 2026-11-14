#include "gran/input/internal/text_cursor.hpp"

namespace gran::input::internal
{
    /*
    ** Шаги по словам для Ctrl/Option+стрелок и стирания слова. Слово — серия символов одного класса, как у двойного
    ** щелчка (wordBounds): буквы и цифры, либо знаки. Пробелы — промежутки, шаг их пропускает. Перевод строки —
    ** остановка: шаг от края строки переходит через него на соседнюю строку и дальше не идёт, поэтому пустые строки
    ** не проскакиваются насквозь (замечание Т-46).
    **
    ** The word steps for Ctrl/Option+arrows and word erasing. A word is a run of characters of one class, as for
    ** a double click (wordBounds): letters and digits, or marks. Spaces are gaps, a step skips them. A line break is
    ** a stop: a step from a line edge crosses it to the neighbour line and goes no further, so empty lines are
    ** not skipped through (remark T-46).
    */
    std::size_t wordStartBefore(const std::string& text, std::size_t caret)
    {
        std::size_t position = caret;
        std::size_t from = 0;
        std::size_t to = 0;

        while (position > 0)
        {
            std::size_t previous = previousBoundary(text, position);
            if (isLineBreak(text, previous) && position == caret)
                return previous;
            if (isLineBreak(text, previous))
                return position;
            if (!isWordGap(text, previous))
            {
                wordBounds(text, previous, from, to);
                return from;
            }
            position = previous;
        }
        return 0;
    }

    // macOS и Linux: промежутки, затем конец слова; перед переводом строки — остановка / macOS and Linux: the gaps, then the word end; a stop before a line break
    std::size_t wordEndAfter(const std::string& text, std::size_t caret)
    {
        std::size_t position = caret;
        std::size_t from = 0;
        std::size_t to = 0;

        while (position < text.size())
        {
            if (isLineBreak(text, position) && position == caret)
                return nextBoundary(text, position);
            if (isLineBreak(text, position))
                return position;
            if (!isWordGap(text, position))
            {
                wordBounds(text, position, from, to);
                return to;
            }
            position = nextBoundary(text, position);
        }
        return text.size();
    }

    // Windows: конец текущего слова, затем промежутки; у края строки — через перевод строки
    // Windows: the current word end, then the gaps; at a line edge — across the line break
    std::size_t wordNextStart(const std::string& text, std::size_t caret)
    {
        std::size_t position = caret;
        std::size_t from = 0;
        std::size_t to = 0;

        if (isLineBreak(text, position))
            return nextBoundary(text, position);
        if (position < text.size() && !isWordGap(text, position))
        {
            wordBounds(text, position, from, to);
            position = to;
        }
        while (isWordGap(text, position))
            position = nextBoundary(text, position);
        return position;
    }

    /*
    ** Начало абзаца (текст между переводами строк). jump — каретка уже в начале: к началу предыдущего абзаца
    ** (Option+↑ на macOS); без jump остаётся на месте (Control+A). Байт '\n' не бывает внутри символа UTF-8.
    **
    ** The paragraph start (the text between line breaks). jump — the caret is already at the start: to the previous
    ** paragraph start (Option+↑ on macOS); without jump it stays (Control+A). The '\n' byte never occurs inside a UTF-8 character.
    */
    std::size_t paragraphStart(const std::string& text, std::size_t caret, bool jump)
    {
        std::size_t position = caret;

        if (position > text.size())
            position = text.size();
        if (jump && position > 0 && isLineBreak(text, position - 1))
            --position;
        while (position > 0 && !isLineBreak(text, position - 1))
            --position;
        return position;
    }

    // Конец абзаца; jump — каретка уже в конце: к концу следующего (Option+↓) / The paragraph end; jump — already at the end: to the next one's end (Option+↓)
    std::size_t paragraphEnd(const std::string& text, std::size_t caret, bool jump)
    {
        std::size_t position = caret;

        if (position > text.size())
            position = text.size();
        if (jump && isLineBreak(text, position))
            ++position;
        while (position < text.size() && !isLineBreak(text, position))
            ++position;
        return position;
    }
}
