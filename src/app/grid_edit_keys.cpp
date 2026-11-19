#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    namespace
    {
        /*
        ** Enter записывает значение. Многострочное поле в шаблоне правки отправляет, как всегда: при submitKey="enter" —
        ** Enter без Shift, иначе Ctrl+Enter (Enter там — новая строка).
        **
        ** Enter writes the value. A multi-line field in the edit template submits as usual: with submitKey="enter" —
        ** Enter without Shift, otherwise Ctrl+Enter (Enter there is a new line).
        */
        bool submits(const ui::Node& value, const platform::Event& event) noexcept
        {
            bool control = (event.modifiers & platform::MODIFIER_CONTROL) != 0;
            bool shift = (event.modifiers & platform::MODIFIER_SHIFT) != 0;

            if (value.kind != ui::ElementKind::TEXT_AREA)
                return true;
            if (value.content.submitKey == ui::SubmitKey::ENTER)
                return !shift;
            return control;
        }
    }

    /*
    ** Клавиши редактора перехватываются до поля: Esc иначе снял бы фокус, Tab — увёл бы его к другому элементу.
    ** Сама правка заканчивается после события, в безопасной точке (settleGridEdit): здесь — только pending (ревью Т-69).
    ** Фокус в слое внутри редактора (раскрытый список ComboBox) — клавиши его: Enter выбирает пункт, а не записывает.
    **
    ** The editor keys are caught before the field: Esc would otherwise drop the focus, Tab would take it to another control.
    ** The edit itself finishes after the event, at the safe point (settleGridEdit): here — only pending (review T-69).
    ** The focus in a layer inside the editor (an open ComboBox list) — the keys are its own: Enter picks an item rather than writes.
    */
    bool gridEditorKey(AppWindow& win, ui::Node& focus, const platform::Event& event)
    {
        if (!win.gridEdit || win.gridEdit->editor == nullptr || !insideNode(&focus, win.gridEdit->editor)
            || ui::popupOf(&focus) != ui::popupOf(win.gridEdit->editor))
            return false;
        GridEdit& edit = *win.gridEdit;
        if (event.key == platform::Key::ESCAPE)
            edit.pending = GridEditEnd::CANCEL;
        else if (event.key == platform::Key::TAB && (event.modifiers & platform::MODIFIER_CONTROL) == 0)
        {
            edit.pending = GridEditEnd::SUBMIT;
            edit.tab = GridTab::FORWARD;
            if ((event.modifiers & platform::MODIFIER_SHIFT) != 0)
                edit.tab = GridTab::BACKWARD;
        }
        else if (event.key == platform::Key::ENTER && submits(*edit.value, event))
            edit.pending = GridEditEnd::SUBMIT;
        else
            return false;
        return true;
    }

    // Двойной щелчок по ячейке строки (не по распорке или промежутку) / A double click on a row cell (not on a spacer or a gap)
    bool gridDoubleClick(AppWindow& win, ui::Node& rows, const ui::Node& hit)
    {
        const ui::Node* row = &hit;

        while (row != nullptr && row->parent != &rows)
            row = row->parent;
        if (row == nullptr || row->kind != ui::ElementKind::GRID_ROW || ui::cellColumn(*row, hit) == ui::NO_COLUMN)
            return false;
        return beginGridEdit(win, rows, nullptr);
    }
}
