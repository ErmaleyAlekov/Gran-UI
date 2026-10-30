#include "gran/undo/undo_stack.hpp"

#include <utility>

namespace gran::undo
{
    // Пустой обработчик допустим: такая сторона шага ничего не делает / An empty handler is allowed: that side of the step does nothing
    ActionStep::ActionStep(std::function<void()> undoHandler, std::function<void()> redoHandler)
        : _undo(std::move(undoHandler)), _redo(std::move(redoHandler))
    {
    }

    void ActionStep::undo()
    {
        if (_undo)
            _undo();
    }

    void ActionStep::redo()
    {
        if (_redo)
            _redo();
    }
}
