#include "gran/input/input_router.hpp"
#include "gran/input/internal/text_cursor.hpp"
#include "gran/input/internal/text_history.hpp"
#include "gran/utils/utf8.hpp"

namespace gran::input
{
    namespace
    {
        /*
        ** Вставляемый текст по виду поля: в однострочном переводы строк и табуляции — пробелы,
        ** в многострочном \r\n и одиночный \r — перевод строки \n (буфер обмена Windows хранит \r\n).
        **
        ** The inserted text by the field kind: in a single-line one the line breaks and tabs are spaces,
        ** in a multi-line one \r\n and a lone \r are the line break \n (the Windows clipboard keeps \r\n).
        */
        std::string fieldInput(const ui::Node& input, const std::string& text)
        {
            bool multiLine = input.kind == ui::ElementKind::TEXT_AREA;
            std::string clean;

            clean.reserve(text.size());
            for (std::size_t index = 0; index < text.size(); ++index)
            {
                char symbol = text[index];
                if (multiLine && symbol == '\r' && index + 1 < text.size() && text[index + 1] == '\n')
                    continue;
                if (multiLine && symbol == '\r')
                    symbol = '\n';
                else if (!multiLine && (symbol == '\n' || symbol == '\r' || symbol == '\t'))
                    symbol = ' ';
                clean += symbol;
            }
            return clean;
        }

        // Вставка вместо выделения; maxLength — в символах после замены / An insertion instead of the selection; maxLength is in characters after the replacement
        bool insertAs(RouterState& state, ui::Node& input, const std::string& text, internal::EditKind kind)
        {
            ui::NodeContent& content = input.content;
            std::string clean = fieldInput(input, text);
            std::size_t from = content.caret;
            std::size_t to = content.caret;
            std::size_t limit = content.maxLength;

            internal::selectionRange(content, from, to);
            std::size_t kept = countUtf8(content.text) - countUtf8(content.text.substr(from, to - from));
            if (clean.empty() || content.readOnly || (limit > 0 && kept + countUtf8(clean) > limit))
                return false;
            return internal::applyEdit(state, input, internal::TextEdit{from, to, clean, kind});
        }
    }

    // Вставка из буфера обмена, меню, доступности — отдельный шаг отмены / An insertion from the clipboard, the menu, accessibility — a separate undo step
    bool insertText(RouterState& state, ui::Node& input, const std::string& text)
    {
        return insertAs(state, input, text, internal::EditKind::OTHER);
    }

    /*
    ** Набор: символ за символом в один шаг. Перевод строки (Enter в TextArea идёт через insertText)
    ** и замена выделения тоже начинают шаг, но последующий набор вливается в него.
    **
    ** Typing: character by character into one step. A line break (Enter in a TextArea goes through insertText)
    ** and a replacement of the selection start a step too, but the following typing flows into it.
    */
    bool internal::typeText(RouterState& state, ui::Node& input, const std::string& text)
    {
        return insertAs(state, input, text, internal::EditKind::TYPING);
    }
}
