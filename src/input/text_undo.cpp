#include "gran/input/internal/text_history.hpp"
#include "gran/input/internal/text_cursor.hpp"

namespace gran::input::internal
{
    // Отмена в поле только для чтения не идёт: его текст правит лишь код / Undo does not run in a read-only field: only the code edits its text
    bool undoText(RouterState& state, ui::Node& input)
    {
        if (!canUndoText(input))
            return false;
        input.content.history->steps.undo();
        textChanged(state, input);
        return true;
    }

    bool redoText(RouterState& state, ui::Node& input)
    {
        if (!canRedoText(input))
            return false;
        input.content.history->steps.redo();
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
