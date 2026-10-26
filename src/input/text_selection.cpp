#include "gran/input/internal/text_cursor.hpp"

#include <algorithm>

namespace gran::input::internal
{
    // Меньший и больший из концов; пустое выделение — false / The smaller and the larger end; an empty selection — false
    bool selectionRange(const ui::NodeContent& content, std::size_t& from, std::size_t& to) noexcept
    {
        std::size_t size = content.text.size();

        from = std::min(std::min(content.anchor, content.caret), size);
        to = std::min(std::max(content.anchor, content.caret), size);
        return from < to;
    }

    // Изменение каретки или выделения видно только на кадре: DIRTY_PAINT / A caret or selection change shows only in the frame: DIRTY_PAINT
    void placeCaret(ui::Node& node, std::size_t position, bool extend)
    {
        node.content.caret = std::min(position, node.content.text.size());
        if (!extend)
            node.content.anchor = node.content.caret;
        ui::markDirty(node, ui::DIRTY_PAINT);
    }

    // Выделенное удаляется, каретка встаёт на место начала / The selected part is erased, the caret stands at its start
    bool eraseSelection(RouterState& state, ui::Node& input)
    {
        std::size_t from = 0;
        std::size_t to = 0;

        if (input.content.readOnly || !selectionRange(input.content, from, to))
            return false;
        input.content.text.erase(from, to - from);
        input.content.caret = from;
        input.content.anchor = from;
        textChanged(state, input);
        return true;
    }

    /*
    ** Якорь — в начале слова, каретка — в конце (как двойной щелчок в браузере). В поле пароля
    ** выделяется всё: границы слов выдали бы, где в пароле пробелы и знаки.
    **
    ** The anchor at the word start, the caret at its end (as a browser double click). In a password input
    ** everything is selected: word bounds would reveal where the password has spaces and marks.
    */
    void selectWord(ui::Node& node, std::size_t position)
    {
        std::size_t from = 0;
        std::size_t to = node.content.text.size();

        if (!node.content.password)
            wordBounds(node.content.text, position, from, to);
        node.content.anchor = from;
        node.content.caret = to;
        ui::markDirty(node, ui::DIRTY_PAINT);
    }
}
