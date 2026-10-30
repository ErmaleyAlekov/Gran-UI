#include "gran/input/internal/text_history.hpp"

namespace gran::input::internal
{
    /*
    ** Продолжает ли правка next шаг, кончающийся в этих границах. Набор — next ничего не удаляет и стоит
    ** сразу за вставленным; Backspace — next удаляет символ прямо перед позицией шага; Delete — next
    ** удаляет на той же позиции. Новая строка и прочее (OTHER) не объединяются.
    **
    ** Whether the next edit continues a step ending in these bounds. Typing — next removes nothing and stands
    ** right after the inserted part; Backspace — next removes the character right before the step position; Delete — next
    ** removes at the same position. A new line and the rest (OTHER) do not merge.
    */
    static bool continues(EditKind kind, std::size_t position, std::size_t insertedEnd, const TextEdit& next)
    {
        if (next.kind != kind)
            return false;
        if (kind == EditKind::TYPING)
            return next.from == next.to && next.from == insertedEnd;
        if (kind == EditKind::ERASE_BACK)
            return next.inserted.empty() && next.to == position;
        if (kind == EditKind::ERASE_FORWARD)
            return next.inserted.empty() && next.from == position;
        return false;
    }

    /*
    ** Набор дописывает вставленное, Backspace растит удалённое влево (позиция сдвигается), Delete — вправо.
    ** Каретка «до» остаётся от первой правки: отмена вернёт её туда, где начался набор.
    **
    ** Typing appends to the inserted part, Backspace grows the removed part to the left (the position moves), Delete — to the right.
    ** The caret "before" stays from the first edit: undo returns it where the typing began.
    */
    bool TextStep::mergeWith(const undo::UndoStep& next)
    {
        const TextStep& step = static_cast<const TextStep&>(next);
        TextEdit edit{step._position, step._position + step._removed.size(), step._inserted, step._kind};

        if (&step._field != &_field || step._time - _time > TYPING_PAUSE)
            return false;
        if (!continues(_kind, _position, _position + _inserted.size(), edit))
            return false;
        if (_kind == EditKind::TYPING)
            _inserted += step._inserted;
        else if (_kind == EditKind::ERASE_BACK)
        {
            _removed.insert(0, step._removed);
            _position = step._position;
        }
        else
            _removed += step._removed;
        _time = step._time;
        return true;
    }
}
