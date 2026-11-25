#include "gran/input/internal/text_history.hpp"

#include <stdexcept>
#include <utility>

namespace gran::input::internal
{
    namespace
    {
        // Вид объединения шагов текста (в стеке поля других видов нет) / The text step merge kind (a field stack has no other kinds)
        constexpr int TEXT_MERGE_ID = 1;

        // Замена участка и новое выделение; ширина текста влияет на компоновку / Replacing a part and the new selection; the text width affects layout
        ui::NodeContent& replacePart(ui::TextHistory& history, std::size_t position, const std::string& before,
            const std::string& after)
        {
            if (history.activeField == nullptr)
                throw std::logic_error("Text history has no active field");
            ui::Node& field = *history.activeField;
            ui::NodeContent& content = field.content;

            if (position > content.text.size() || before.size() > content.text.size() - position
                || content.text.compare(position, before.size(), before) != 0)
                throw std::logic_error("Text history does not match the active field");
            std::string next = content.text;
            next.replace(position, before.size(), after);
            std::string expected = next;
            content.text.swap(next);
            history.expectedText.swap(expected);
            ui::markDirty(field, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            return content;
        }
    }

    TextStep::TextStep(ui::TextHistory& history, const ui::Node& field, const TextEdit& edit, std::string removed)
        : _history(history), _position(edit.from), _removed(std::move(removed)), _inserted(edit.inserted),
          _caretBefore(field.content.caret), _anchorBefore(field.content.anchor), _kind(edit.kind),
          _time(std::chrono::steady_clock::now())
    {
    }

    // Вставленное заменяется удалённым, выделение — как до правки / The inserted part is replaced with the removed one, the selection is as before the edit
    void TextStep::undo()
    {
        auto& content = replacePart(_history, _position, _inserted, _removed);
        content.caret = _caretBefore;
        content.anchor = _anchorBefore;
    }

    // Как при правке: каретка за вставленным, выделения нет / As on the edit: the caret after the inserted part, no selection
    void TextStep::redo()
    {
        auto& content = replacePart(_history, _position, _removed, _inserted);
        content.caret = _position + _inserted.size();
        content.anchor = content.caret;
    }

    int TextStep::mergeId() const noexcept
    {
        return TEXT_MERGE_ID;
    }
}
