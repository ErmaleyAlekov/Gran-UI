#include "gran/input/internal/text_cursor.hpp"

namespace gran::input::internal
{
    // Копируется только выделение; поле пароля текст не отдаёт / Only the selection is copied; a password input does not give its text away
    bool copySelection(RouterState& state, const ui::Node& node)
    {
        std::size_t from = 0;
        std::size_t to = 0;

        if (state.window == nullptr || node.content.password || !selectionRange(node.content, from, to))
            return false;
        state.window->setClipboardText(node.content.text.substr(from, to - from));
        return true;
    }

    /*
    ** Вырезание — копия выделения и его удаление; вставка заменяет выделение (insertText).
    ** Ctrl+C и Ctrl+A обрабатывает общая навигация по тексту (navigateText).
    **
    ** Cutting is a copy of the selection and its erasure; a paste replaces the selection (insertText).
    ** Ctrl+C and Ctrl+A are handled by the common text navigation (navigateText).
    */
    bool clipboardEdit(RouterState& state, ui::Node& input, platform::Key key)
    {
        if (key == platform::Key::V && state.window != nullptr)
            return insertText(state, input, state.window->clipboardText());
        if (key == platform::Key::X && !input.content.readOnly && copySelection(state, input))
            return eraseSelection(state, input);
        return false;
    }
}
