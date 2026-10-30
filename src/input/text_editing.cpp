#include "gran/input/input_router.hpp"
#include "gran/input/internal/text_cursor.hpp"
#include "gran/input/internal/text_history.hpp"

namespace gran::input
{
    namespace
    {
        // Удаление [from, to) одним шагом вида kind; каретка и якорь — в from / Erasing [from, to) as one step of the kind; the caret and the anchor go to from
        bool eraseRange(RouterState& state, ui::Node& input, std::size_t from, std::size_t to, internal::EditKind kind)
        {
            return internal::applyEdit(state, input, internal::TextEdit{from, to, std::string(), kind});
        }

        /*
        ** Ctrl+Z — отмена, Ctrl+Y и Ctrl+Shift+Z — повтор (как в Windows и в браузерах). Сочетания
        ** зарезервированы за полем (command_keys.cpp), команды <Command> их здесь не перехватывают.
        **
        ** Ctrl+Z — undo, Ctrl+Y and Ctrl+Shift+Z — redo (as in Windows and in browsers). The chords
        ** are reserved for the field (command_keys.cpp), the <Command> commands do not intercept them here.
        */
        bool historyKey(RouterState& state, ui::Node& input, const platform::Event& event, bool& handled)
        {
            bool shift = (event.modifiers & platform::MODIFIER_SHIFT) != 0;

            // Ctrl+Alt — это AltGr (набор знаков), не отмена / Ctrl+Alt is AltGr (typing marks), not undo
            handled = (event.modifiers & platform::MODIFIER_ALT) == 0
                && (event.key == platform::Key::Y || event.key == platform::Key::Z);
            if (!handled)
                return false;
            if (event.key == platform::Key::Z && !shift)
                return internal::undoText(state, input);
            return internal::redoText(state, input);
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
    ** движение каретки и выделение (navigateText), Ctrl+Z/Y — отмена и повтор, Ctrl+X/V — буфер обмена.
    ** Каретка всегда стоит на границе символа UTF-8.
    **
    ** The input keys: erasing backward and forward (with a selection — the selection itself), Enter (enterKey),
    ** moving the caret and selecting (navigateText), Ctrl+Z/Y — undo and redo, Ctrl+X/V — the clipboard.
    ** The caret always stands on a UTF-8 character boundary.
    */
    bool editText(RouterState& state, ui::Node& input, const platform::Event& event)
    {
        const std::string& text = input.content.text;
        std::size_t caret = input.content.caret;

        bool handled = false;

        switch (event.key)
        {
            case platform::Key::BACKSPACE:
                return internal::eraseSelection(state, input)
                    || eraseRange(state, input, internal::previousBoundary(text, caret), caret, internal::EditKind::ERASE_BACK);
            case platform::Key::DELETE_KEY:
                return internal::eraseSelection(state, input)
                    || eraseRange(state, input, caret, internal::nextBoundary(text, caret), internal::EditKind::ERASE_FORWARD);
            case platform::Key::ENTER:
                return enterKey(state, input, event.modifiers);
            default:
                break;
        }
        if (navigateText(state, input, event))
            return true;
        if ((event.modifiers & platform::MODIFIER_CONTROL) == 0)
            return false;
        bool changed = historyKey(state, input, event, handled);
        if (handled)
            return changed;
        return internal::clipboardEdit(state, input, event.key);
    }
}
