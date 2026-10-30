#include "gran/input/internal/text_history.hpp"

#include <utility>

namespace gran::input::internal
{
    namespace
    {
        // Вид объединения шагов текста (в стеке поля других видов нет) / The text step merge kind (a field stack has no other kinds)
        constexpr int TEXT_MERGE_ID = 1;

        // Замена участка и новое выделение; ширина текста влияет на компоновку / Replacing a part and the new selection; the text width affects layout
        void replacePart(ui::Node& field, std::size_t position, std::size_t length, const std::string& text)
        {
            ui::NodeContent& content = field.content;

            content.text.replace(position, length, text);
            if (content.history)
                content.history->textSize = content.text.size();
            ui::markDirty(field, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        }
    }

    TextStep::TextStep(ui::Node& field, const TextEdit& edit, std::string removed)
        : _field(field), _position(edit.from), _removed(std::move(removed)), _inserted(edit.inserted),
          _caretBefore(field.content.caret), _anchorBefore(field.content.anchor), _kind(edit.kind),
          _time(std::chrono::steady_clock::now())
    {
    }

    // Вставленное заменяется удалённым, выделение — как до правки / The inserted part is replaced with the removed one, the selection is as before the edit
    void TextStep::undo()
    {
        replacePart(_field, _position, _inserted.size(), _removed);
        _field.content.caret = _caretBefore;
        _field.content.anchor = _anchorBefore;
    }

    // Как при правке: каретка за вставленным, выделения нет / As on the edit: the caret after the inserted part, no selection
    void TextStep::redo()
    {
        replacePart(_field, _position, _removed.size(), _inserted);
        _field.content.caret = _position + _inserted.size();
        _field.content.anchor = _field.content.caret;
    }

    int TextStep::mergeId() const noexcept
    {
        return TEXT_MERGE_ID;
    }
}
