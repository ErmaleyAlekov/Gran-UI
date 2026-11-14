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
        ** Backspace и Delete: при выделении стирается оно, иначе — от каретки до границы по смыслу клавиши
        ** (символ, слово, край строки — internal::eraseMove в соглашениях ОС). Чужое сочетание — не обработано.
        **
        ** Backspace and Delete: with a selection it is erased, otherwise — from the caret to the bound by the key meaning
        ** (a character, a word, the line edge — internal::eraseMove in the OS conventions). A foreign chord is not handled.
        */
        bool eraseKey(RouterState& state, ui::Node& input, const platform::Event& event)
        {
            internal::TextMove move = internal::eraseMove(event.key, event.modifiers, platform::hostOs());
            std::size_t caret = input.content.caret;
            std::size_t target = caret;

            if (move == internal::TextMove::NONE)
                return false;
            if (internal::eraseSelection(state, input))
                return true;
            if (!internal::moveTarget(state, input, move, target))
                return false;
            if (target < caret)
                return eraseRange(state, input, target, caret, internal::EditKind::ERASE_BACK);
            return eraseRange(state, input, caret, target, internal::EditKind::ERASE_FORWARD);
        }

        /*
        ** Ctrl+Z — отмена, Ctrl+Y и Ctrl+Shift+Z — повтор (как в Windows и в браузерах; на macOS повтор — только
        ** Cmd+Shift+Z, Cmd+Y — команда программы). Сочетания зарезервированы за полем (command_keys.cpp).
        **
        ** Ctrl+Z — undo, Ctrl+Y and Ctrl+Shift+Z — redo (as in Windows and in browsers; on macOS redo is only
        ** Cmd+Shift+Z, Cmd+Y is a program command). The chords are reserved for the field (command_keys.cpp).
        */
        bool historyKey(RouterState& state, ui::Node& input, const platform::Event& event, bool& handled)
        {
            bool shift = (event.modifiers & platform::MODIFIER_SHIFT) != 0;

            // Ctrl+Alt — это AltGr (набор знаков), не отмена / Ctrl+Alt is AltGr (typing marks), not undo
            bool redoKey = event.key == platform::Key::Y && platform::hostOs() != platform::HostOs::MACOS;

            handled = (event.modifiers & platform::MODIFIER_ALT) == 0 && (redoKey || event.key == platform::Key::Z);
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
        bool handled = false;

        if (internal::eraseMove(event.key, event.modifiers, platform::hostOs()) != internal::TextMove::NONE)
            return eraseKey(state, input, event);
        if (event.key == platform::Key::ENTER)
            return enterKey(state, input, event.modifiers);
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
