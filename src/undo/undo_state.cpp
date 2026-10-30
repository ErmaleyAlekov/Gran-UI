#include "gran/undo/undo_stack.hpp"

namespace gran::undo
{
    bool UndoStack::canUndo() const noexcept
    {
        return _index > 0;
    }

    bool UndoStack::canRedo() const noexcept
    {
        return _index < _steps.size();
    }

    bool UndoStack::isClean() const noexcept
    {
        return _clean == _index;
    }

    // Сохранение закрывает верхний шаг: следующая правка — уже после сохранения / Saving closes the top step: the next edit is after the saving
    void UndoStack::setClean()
    {
        _clean = _index;
        _merge = false;
        notify();
    }

    void UndoStack::breakMerge() noexcept
    {
        _merge = false;
    }
}
