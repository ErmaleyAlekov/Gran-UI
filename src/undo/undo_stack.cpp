#include "gran/undo/undo_stack.hpp"

namespace gran::undo
{
    UndoStack::UndoStack(std::size_t limit) : _limit(limit)
    {
    }

    /*
    ** Позиция сдвигается до вызова шага, а при исключении возвращается: стек остаётся согласованным,
    ** ошибку видит вызывающий. Отмена закрывает верхний шаг для объединения.
    **
    ** The position moves before the step call and comes back on an exception: the stack stays consistent,
    ** the caller sees the error. Undo closes the top step for merging.
    */
    bool UndoStack::undo()
    {
        if (_busy || _group || !canUndo())
            return false;
        const auto progress = pendingRedoProgress();
        if (!progress) --_index;
        _busy = true;
        try
        {
            _steps[_index]->undo();
        }
        catch (...)
        {
            _busy = false;
            if (!progress) ++_index;
            else if (pendingRedoProgress() != progress) notify();
            throw;
        }
        _busy = false;
        _merge = false;
        notify();
        return true;
    }

    // Зеркально undo / Mirrors undo
    bool UndoStack::redo()
    {
        if (_busy || _group || _index >= _steps.size())
            return false;
        const auto previous = pendingRedoProgress();
        _busy = true;
        try
        {
            _steps[_index]->redo();
        }
        catch (...)
        {
            _busy = false;
            if (pendingRedoProgress() != previous) notify();
            throw;
        }
        _busy = false;
        ++_index;
        _merge = false;
        notify();
        return true;
    }

    void UndoStack::clear()
    {
        if (_busy || _group)
            return;
        _steps.clear();
        _index = 0;
        _clean = 0;
        _cleanProgress = 0;
        _merge = false;
        notify();
    }

    void UndoStack::notify()
    {
        if (_observer != nullptr)
            _observer->undoChanged(*this);
    }
}
