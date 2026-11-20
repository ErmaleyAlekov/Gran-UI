#include "gran/app/internal/app_state.hpp"
#include "gran/data/cell_value.hpp"

namespace gran::app
{
    namespace
    {
        // Событие адресовано ячейке редактора; строку убрали — таблице; ушла и таблица — некому / The event goes to the editor cell; the row was removed — to the table; the table is gone too — to nobody
        ui::Node* cellTarget(const GridEdit& edit) noexcept
        {
            if (edit.editor != nullptr && edit.editor->parent != nullptr)
                return edit.editor->parent;
            return edit.grid;
        }

        /*
        ** Отказ обработчика: состояние Invalid и текст ошибки подсказкой у поля значения — его покажет наведение,
        ** а диктор прочтёт как справку (HelpText). Без текста — прежняя подсказка поля остаётся.
        **
        ** The handler refusal: the Invalid state and the error text as the tooltip of the value node — hovering shows it,
        ** and a screen reader reads it as the help (HelpText). Without a text the former input tooltip stays.
        */
        void showCellError(GridEdit& edit, const std::string& error)
        {
            if (edit.value == nullptr)
                return;
            if (!error.empty())
                edit.value->content.tooltip = error;
            markGridEditInvalid(*edit.value, true);
        }
    }

    /*
    ** Событие cellEdit (отменяемое, всплывает от ячейки к таблице и выше): код программы видит запись, поле, прежнее
    ** и новое значения и может отказать (preventDefault или cell.error) либо заменить значение (cell.value — оно снова
    ** проверяется типом столбца). Вызывается в безопасной точке, после проверки типа и до записи в модель.
    **
    ** The cellEdit event (cancelable, it bubbles from the cell to the table and up): the program code sees the record, the field,
    ** the former and the new values and may refuse (preventDefault or cell.error) or replace the value (cell.value — it is checked
    ** by the column type again). It is called at a safe point, after the type check and before writing into the model.
    */
    void markGridEditInvalid(ui::Node& value, bool invalid)
    {
        std::uint16_t bit = style::stateBit(style::State::INVALID);
        std::uint16_t mask = static_cast<std::uint16_t>(value.stateMask & ~bit);

        if (invalid)
            mask = static_cast<std::uint16_t>(mask | bit);
        if (mask == value.stateMask)
            return;
        value.stateMask = mask;
        ui::markDirty(value, ui::DIRTY_STYLE | ui::DIRTY_PAINT);
    }

    bool approveCellEdit(AppWindow& win, std::string& text)
    {
        GridEdit& edit = *win.gridEdit;
        ui::Node* target = cellTarget(edit);

        if (target == nullptr || !win.router || !edit.record->alive())
            return true;
        ui::UiEvent event = ui::makeEvent(ui::EventType::CELL_EDIT, *target, std::string());
        event.cell.row = edit.record->index();
        event.cell.field = edit.field;
        event.cell.oldValue = edit.original;
        event.cell.value = text;
        win.router->dispatch(event);
        if (win.gridEdit.get() != &edit)
            return false;
        bool allowed = !event.defaultPrevented && event.cell.error.empty();
        if (allowed && event.cell.value != text)
        {
            text = event.cell.value;
            allowed = data::normalizeValue(edit.type, text);
        }
        if (!allowed)
            showCellError(edit, event.cell.error);
        return allowed;
    }
}
