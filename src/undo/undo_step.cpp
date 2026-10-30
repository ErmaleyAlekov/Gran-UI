#include "gran/undo/undo_stack.hpp"

namespace gran::undo
{
    // Деструктор в файле реализации: заголовки без исполняемого кода / The destructor in the implementation file: headers carry no executable code
    UndoStep::~UndoStep() = default;

    // Отрицательный вид: шаг стоит отдельно / A negative kind: the step stands alone
    int UndoStep::mergeId() const noexcept
    {
        return -1;
    }

    // Без своего правила шаги не сливаются / Without its own rule the steps do not merge
    bool UndoStep::mergeWith(const UndoStep& next)
    {
        (void)next;
        return false;
    }

    UndoObserver::~UndoObserver() = default;
}
