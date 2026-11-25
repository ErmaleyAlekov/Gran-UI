#include "gran/input/internal/text_history.hpp"
#include "gran/input/internal/text_cursor.hpp"

namespace gran::input::internal
{
    namespace
    {
        // История переживает замену поля: цель нужна только во время операции.
        // History survives field replacement: the target is only needed during the operation.
        void applyHistory(ui::Node& input, bool undo)
        {
            const auto history = input.content.history;
            auto* previous = history->activeField;
            history->activeField = &input;
            try
            {
                if (undo)
                    history->steps.undo();
                else
                    history->steps.redo();
            }
            catch (...)
            {
                history->activeField = previous;
                throw;
            }
            history->activeField = previous;
        }
    }

    // Отмена в поле только для чтения не идёт: его текст правит лишь код / Undo does not run in a read-only field: only the code edits its text
    bool undoText(RouterState& state, ui::Node& input)
    {
        if (!canUndoText(input))
            return false;
        applyHistory(input, true);
        textChanged(state, input);
        return true;
    }

    bool redoText(RouterState& state, ui::Node& input)
    {
        if (!canRedoText(input))
            return false;
        applyHistory(input, false);
        textChanged(state, input);
        return true;
    }

    bool canUndoText(ui::Node& input)
    {
        ui::TextHistory* history = currentHistory(input);

        return history != nullptr && !input.content.readOnly && history->steps.canUndo();
    }

    bool canRedoText(ui::Node& input)
    {
        ui::TextHistory* history = currentHistory(input);

        return history != nullptr && !input.content.readOnly && history->steps.canRedo();
    }
}
