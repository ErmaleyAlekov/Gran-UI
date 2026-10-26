#include "gran/input/input_router.hpp"
#include "gran/input/internal/text_cursor.hpp"
#include "gran/utils/utf8.hpp"

namespace gran::input
{
    namespace
    {
        // Удаление [from, to) и уведомление; каретка и якорь — в from / Erasing [from, to) and the notification; the caret and the anchor go to from
        bool eraseRange(RouterState& state, ui::Node& input, std::size_t from, std::size_t to)
        {
            if (from >= to || input.content.readOnly)
                return false;
            input.content.text.erase(from, to - from);
            input.content.caret = from;
            input.content.anchor = from;
            internal::textChanged(state, input);
            return true;
        }

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

        /*
        ** Enter: однострочное поле — onSubmit; многострочное — отправка или новая строка по submitKey
        ** (ENTER: Enter и Ctrl+Enter отправляют, Shift+Enter — строка; CONTROL_ENTER: отправляет только Ctrl+Enter).
        **
        ** Enter: a single-line input — onSubmit; a multi-line one — submitting or a new line by submitKey
        ** (ENTER: Enter and Ctrl+Enter submit, Shift+Enter is a line; CONTROL_ENTER: only Ctrl+Enter submits).
        */
        bool enterKey(RouterState& state, ui::Node& input, std::uint8_t modifiers)
        {
            bool control = (modifiers & platform::MODIFIER_CONTROL) != 0;
            bool shift = (modifiers & platform::MODIFIER_SHIFT) != 0;
            bool submit = input.kind == ui::ElementKind::INPUT || control;

            if (input.kind == ui::ElementKind::TEXT_AREA && input.content.submitKey == ui::SubmitKey::ENTER && !shift)
                submit = true;
            if (!submit)
            {
                // Только чтение или предел maxLength: строка не вставится, но клавиша обработана
                // Read-only or the maxLength limit: the line is not inserted, but the key is handled
                insertText(state, input, "\n");
                return true;
            }
            if (!input.content.onSubmit.empty())
                state.actions->action(input.content.onSubmit, input);
            return true;
        }
    }

    /*
    ** Клавиши поля ввода: удаление назад и вперёд (при выделении — само выделение), Enter (enterKey),
    ** движение каретки и выделение (navigateText), Ctrl+X/V — буфер обмена.
    ** Каретка всегда стоит на границе символа UTF-8.
    **
    ** The input keys: erasing backward and forward (with a selection — the selection itself), Enter (enterKey),
    ** moving the caret and selecting (navigateText), Ctrl+X/V — the clipboard.
    ** The caret always stands on a UTF-8 character boundary.
    */
    bool editText(RouterState& state, ui::Node& input, const platform::Event& event)
    {
        const std::string& text = input.content.text;
        std::size_t caret = input.content.caret;

        switch (event.key)
        {
            case platform::Key::BACKSPACE:
                return internal::eraseSelection(state, input) || eraseRange(state, input, internal::previousBoundary(text, caret), caret);
            case platform::Key::DELETE_KEY:
                return internal::eraseSelection(state, input) || eraseRange(state, input, caret, internal::nextBoundary(text, caret));
            case platform::Key::ENTER:
                return enterKey(state, input, event.modifiers);
            default:
                break;
        }
        if (navigateText(state, input, event))
            return true;
        if ((event.modifiers & platform::MODIFIER_CONTROL) == 0)
            return false;
        return internal::clipboardEdit(state, input, event.key);
    }

    // Вставка вместо выделения; maxLength — в символах после замены / An insertion instead of the selection; maxLength is in characters after the replacement
    bool insertText(RouterState& state, ui::Node& input, const std::string& text)
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
        content.text.replace(from, to - from, clean);
        content.caret = from + clean.size();
        content.anchor = content.caret;
        internal::textChanged(state, input);
        return true;
    }
}
