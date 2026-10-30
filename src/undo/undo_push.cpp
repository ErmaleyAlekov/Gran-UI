#include "gran/undo/undo_stack.hpp"

#include <utility>

namespace gran::undo
{
    /*
    ** Новый шаг отбрасывает отменённые; затем — объединение с верхним или новая позиция и предел.
    ** Шаг из обработчика undo/redo (стек занят) не записывается.
    **
    ** A new step drops the undone ones; then — merging with the top one or a new position and the limit.
    ** A step from an undo/redo handler (the stack is busy) is not recorded.
    */
    void UndoStack::push(std::unique_ptr<UndoStep> step)
    {
        if (!step || _busy)
            return;
        dropUndone();
        if (tryMerge(*step))
        {
            notify();
            return;
        }
        _steps.push_back(std::move(step));
        _index = _steps.size();
        _merge = true;
        trim();
        notify();
    }

    bool UndoStack::busy() const noexcept
    {
        return _busy;
    }

    // «Чистая» позиция среди отбрасываемых шагов становится недостижимой / A clean position among the dropped steps becomes unreachable
    void UndoStack::dropUndone()
    {
        if (_index == _steps.size())
            return;
        if (_clean != NO_CLEAN && _clean > _index)
            _clean = NO_CLEAN;
        _steps.erase(_steps.begin() + static_cast<std::ptrdiff_t>(_index), _steps.end());
    }

    /*
    ** Как в Qt: на «чистой» позиции не объединять — иначе после сохранения правка влилась бы в шаг
    ** до сохранения, и отмена проскочила бы «чистое» состояние.
    **
    ** As in Qt: do not merge on the clean position — otherwise after saving an edit would flow into a step
    ** before the saving, and undo would skip the clean state.
    */
    bool UndoStack::tryMerge(const UndoStep& step)
    {
        if (!_merge || _index == 0 || _clean == _index)
            return false;
        UndoStep& top = *_steps[_index - 1];
        if (top.mergeId() < 0 || top.mergeId() != step.mergeId())
            return false;
        return top.mergeWith(step);
    }

    // Старейшие шаги уходят первыми; «чистая» позиция сдвигается вместе с ними / The oldest steps go first; the clean position moves together with them
    void UndoStack::trim()
    {
        if (_limit == 0 || _steps.size() <= _limit)
            return;
        std::size_t extra = _steps.size() - _limit;
        _steps.erase(_steps.begin(), _steps.begin() + static_cast<std::ptrdiff_t>(extra));
        _index -= extra;
        if (_clean != NO_CLEAN && _clean < extra)
            _clean = NO_CLEAN;
        else if (_clean != NO_CLEAN)
            _clean -= extra;
    }
}
