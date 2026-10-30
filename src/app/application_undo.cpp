#include "gran/app/internal/app_state.hpp"

#include <utility>

namespace gran
{
    // Связь создаётся вместе со стеком: наблюдатель включён с первого шага / The link is created together with the stack: the observer is on from the first step
    UndoStack& Application::undoStack(const std::string& name)
    {
        std::unique_ptr<app::UndoLink>& link = _state->undoStacks[name];

        if (!link)
            link = std::make_unique<app::UndoLink>(*_state, name);
        return link->stack;
    }

    void Application::pushUndo(Task undoHandler, Task redoHandler, const std::string& stack)
    {
        undoStack(stack).push(std::make_unique<undo::ActionStep>(std::move(undoHandler), std::move(redoHandler)));
    }

    bool Application::undo(const std::string& stack)
    {
        return undoStack(stack).undo();
    }

    bool Application::redo(const std::string& stack)
    {
        return undoStack(stack).redo();
    }

    void Application::onUndoChanged(UndoHandler handler)
    {
        _state->undoHandler = std::move(handler);
    }
}
