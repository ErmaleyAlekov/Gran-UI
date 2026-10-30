#include "gran/undo/undo_stack.hpp"

namespace gran::undo
{
    std::size_t UndoStack::count() const noexcept
    {
        return _steps.size();
    }

    std::size_t UndoStack::index() const noexcept
    {
        return _index;
    }

    std::size_t UndoStack::limit() const noexcept
    {
        return _limit;
    }

    /*
    ** Предел действует сразу. Отменённые шаги считаются вместе с выполненными: при нехватке места
    ** уходят самые старые выполненные (как в Qt — там предел меняется только у пустого стека).
    **
    ** The limit acts at once. The undone steps count together with the done ones: with a lack of room
    ** the oldest done ones go (in Qt the limit can be changed only for an empty stack).
    */
    void UndoStack::setLimit(std::size_t limit)
    {
        std::size_t keep = _index + limit;

        _limit = limit;
        // Отменённых больше, чем помещается: дальние отменённые уходят / More undone steps than fit: the far undone ones go
        if (_limit != 0 && keep < _steps.size())
        {
            _steps.erase(_steps.begin() + static_cast<std::ptrdiff_t>(keep), _steps.end());
            if (_clean != NO_CLEAN && _clean > keep)
                _clean = NO_CLEAN;
        }
        trim();
        notify();
    }

    void UndoStack::setObserver(UndoObserver* observer) noexcept
    {
        _observer = observer;
    }
}
