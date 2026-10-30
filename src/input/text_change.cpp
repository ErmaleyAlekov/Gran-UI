#include "gran/input/internal/text_history.hpp"
#include "gran/input/internal/text_cursor.hpp"

#include <algorithm>
#include <memory>

namespace gran::input::internal
{
    namespace
    {
        /*
        ** Шаг пишется до замены: ему нужны каретка и выделение «до». Поле с undo="false" историю не ведёт
        ** (и прежнюю, если атрибут сменили из кода, отпускает).
        **
        ** The step is recorded before the replacement: it needs the caret and the selection "before". A field with undo="false"
        ** keeps no history (and lets the former one go if the attribute was changed from code).
        */
        void recordEdit(ui::Node& input, const TextEdit& edit)
        {
            ui::TextHistory* history = currentHistory(input);

            if (!input.content.undoable)
            {
                input.content.history.reset();
                return;
            }
            if (history == nullptr)
            {
                input.content.history = std::make_shared<ui::TextHistory>();
                history = input.content.history.get();
                history->steps.setLimit(FIELD_UNDO_LIMIT);
            }
            std::string removed = input.content.text.substr(edit.from, edit.to - edit.from);
            history->steps.push(std::make_unique<TextStep>(input, edit, std::move(removed)));
        }
    }

    // Границы правки ограничены текстом: вызывающие берут их из каретки и выделения / The edit bounds are clamped by the text: the callers take them from the caret and the selection
    bool applyEdit(RouterState& state, ui::Node& input, const TextEdit& edit)
    {
        ui::NodeContent& content = input.content;
        std::size_t to = std::min(edit.to, content.text.size());
        std::size_t from = std::min(edit.from, to);

        if (content.readOnly || (from == to && edit.inserted.empty()))
            return false;
        recordEdit(input, TextEdit{from, to, edit.inserted, edit.kind});
        content.text.replace(from, to - from, edit.inserted);
        content.caret = from + edit.inserted.size();
        content.anchor = content.caret;
        if (content.history)
            content.history->textSize = content.text.size();
        textChanged(state, input);
        return true;
    }

    void breakTyping(ui::Node& node) noexcept
    {
        if (node.content.history)
            node.content.history->steps.breakMerge();
    }

    // Проверка длины дешёвая и ловит почти любую замену текста из кода / The length check is cheap and catches almost any text replacement from code
    ui::TextHistory* currentHistory(ui::Node& input)
    {
        ui::TextHistory* history = input.content.history.get();

        if (history != nullptr && history->textSize != input.content.text.size())
        {
            input.content.history.reset();
            return nullptr;
        }
        return history;
    }
}
