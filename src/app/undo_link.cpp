#include "gran/app/internal/app_state.hpp"

#include <utility>

namespace gran::app
{
    UndoLink::UndoLink(Application::State& state, std::string name) : _state(state), _name(std::move(name))
    {
        stack.setObserver(this);
    }

    // Обработчик копируется: он может сам заменить себя через onUndoChanged / The handler is copied: it may replace itself through onUndoChanged
    void UndoLink::undoChanged(const undo::UndoStack& changed)
    {
        UndoHandler handler = _state.undoHandler;

        refreshUndoCommands(_state, _name);
        if (handler)
            handler(_name, changed);
    }
}
