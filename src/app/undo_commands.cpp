#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    namespace
    {
        constexpr std::size_t PREFIX_LENGTH = 9;

        /*
        ** Действие команды — отмена или повтор: gran:undo, gran:redo (основной стек) или gran:undo:имя.
        ** Другие имена с тем же началом (gran:undone…) — не отмена.
        **
        ** The command action is undo or redo: gran:undo, gran:redo (the main stack) or gran:undo:name.
        ** Other names with the same start (gran:undone…) are not undo.
        */
        bool undoTarget(const ui::Command& command, std::string& stack, bool& redo)
        {
            const std::string& action = command.action;

            if (action.compare(0, PREFIX_LENGTH, "gran:undo") == 0)
                redo = false;
            else if (action.compare(0, PREFIX_LENGTH, "gran:redo") == 0)
                redo = true;
            else
                return false;
            if (action.size() > PREFIX_LENGTH && action[PREFIX_LENGTH] != ':')
                return false;
            stack.clear();
            if (action.size() > PREFIX_LENGTH)
                stack = action.substr(PREFIX_LENGTH + 1);
            return true;
        }

        /*
        ** Есть ли что отменить или повторить. Основной стек делит команду с редактируемым полем окна
        ** (gran:undo сначала отменяет правку поля); стека ещё нет — только поле.
        **
        ** Whether there is something to undo or redo. The main stack shares the command with the edited field of the window
        ** (gran:undo undoes a field edit first); no stack yet — only the field.
        */
        bool stackAllows(const Application::State& state, AppWindow& win, const std::string& stack, bool redo)
        {
            auto found = state.undoStacks.find(stack);

            if (stack.empty() && win.router && win.router->canUndoField(redo))
                return true;
            if (found == state.undoStacks.end())
                return false;
            if (redo)
                return found->second->stack.canRedo();
            return found->second->stack.canUndo();
        }
    }

    void linkUndoCommands(Application::State& state, AppWindow& win)
    {
        std::string stack;
        bool redo = false;

        for (ui::Command& command : win.document.commands)
        {
            if (undoTarget(command, stack, redo))
                command.enabled = stackAllows(state, win, stack, redo);
        }
    }

    // Узлы обходятся только у изменившихся команд: набор в стеке не перекраивает всё дерево / The nodes are walked only for the changed commands: a push does not rework the whole tree
    void refreshUndoCommands(Application::State& state, const std::string& name)
    {
        std::string stack;
        bool redo = false;

        for (const std::unique_ptr<AppWindow>& win : state.windows)
        {
            for (ui::Command& command : win->document.commands)
            {
                if (!undoTarget(command, stack, redo) || stack != name || command.enabled == stackAllows(state, *win, stack, redo))
                    continue;
                command.enabled = !command.enabled;
                refreshCommand(state, command.id);
            }
        }
    }
}
