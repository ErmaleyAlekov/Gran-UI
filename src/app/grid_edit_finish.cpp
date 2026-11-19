#include "gran/app/internal/app_state.hpp"
#include "gran/data/cell_value.hpp"

#include <utility>

namespace gran::app
{
    namespace
    {
        /*
        ** Убрать редактор и вернуть спрятанное содержимое ячейки. Фокус был в редакторе (или пропал вместе
        ** со строкой) — он возвращается списку строк: стрелки снова ходят по таблице. Ушёл в другое место
        ** (щелчок мимо) — остаётся там.
        **
        ** Remove the editor and bring back the hidden cell content. The focus was in the editor (or vanished together
        ** with the row) — it returns to the row list: the arrows walk the table again. It went elsewhere
        ** (a click elsewhere) — it stays there.
        */
        void closeEditor(AppWindow& win, GridEdit& edit)
        {
            ui::Node* editor = edit.editor;
            const ui::Node* focus = nullptr;

            if (win.router)
                focus = win.router->focused();
            bool refocus = insideNode(focus, editor) || (edit.lost && focus == nullptr);
            for (ui::Node* node : edit.hidden)
            {
                node->visible = true;
                ui::markDirty(*node, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            }
            edit.hidden.clear();
            edit.editor = nullptr;
            edit.value = nullptr;
            if (editor != nullptr)
                win.shared.lists.removeNode(*editor);
            if (refocus && edit.rows != nullptr && win.router)
                win.router->focus(edit.rows);
        }

        /*
        ** Значение уходит в запись, если она ещё есть и значение отличается от нынешнего в модели (а не только от
        ** исходного: модель могли сменить, пока шла правка). Сравнение — в приведённом виде: флажок, который
        ** не трогали, не превращает пустое поле в «false». Шаг отмены пишется до записи — пока номер записи
        ** в источнике прокси ещё прежний.
        **
        ** The value goes into the record if it still exists and the value differs from the current one in the model (not only
        ** from the original: the model might change during the edit). The comparison is in the normalized form: an untouched
        ** check box does not turn an empty field into "false". The undo step is recorded before writing — while the record
        ** index in the proxy source is still the former one.
        */
        void writeValue(AppWindow& win, const GridEdit& edit, const std::string& text)
        {
            if (!edit.record->alive())
                return;
            data::ListSource& source = edit.record->source();
            std::size_t index = edit.record->index();
            const data::Record& record = source.at(index);
            std::string before;
            auto found = record.find(edit.field);
            if (found != record.end())
                before = found->second;
            std::string normalized = before;
            if (data::normalizeValue(edit.type, normalized) && normalized == text)
                return;
            pushCellStep(win, edit, index, before, text);
            source.set(index, edit.field, text);
        }

        // Состояние Invalid узла значения: стиль темы показывает ошибку / The Invalid state of the value node: the theme style shows the error
        void markInvalid(ui::Node& value, bool invalid)
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
    }

    /*
    ** Сеанс уходит из окна до удаления редактора и записи: удаление и смена фокуса сообщают окну, а наблюдатели
    ** модели (код программы) могут начать новую правку — им не должен попасться закрываемый сеанс.
    ** Значение потерянного редактора (строку убрали) с ошибкой проверки не записывается.
    **
    ** The session leaves the window before the editor is removed and the value is written: the removal and the focus change notify
    ** the window, and the model observers (the program code) may start a new edit — they must not meet the closing session.
    ** The value of a lost editor (the row was removed) with a check error is not written.
    */
    bool finishGridEdit(AppWindow& win, bool commit)
    {
        if (!win.gridEdit)
            return true;
        GridEdit& edit = *win.gridEdit;
        std::string text = edit.lostValue;
        if (!edit.lost && edit.value != nullptr)
            text = readEditorValue(*edit.value);
        bool valid = data::normalizeValue(edit.type, text);
        if (commit && !valid && edit.value != nullptr)
        {
            markInvalid(*edit.value, true);
            return false;
        }
        std::unique_ptr<GridEdit> done = std::move(win.gridEdit);
        closeEditor(win, *done);
        if (commit && valid)
            writeValue(win, *done, text);
        return true;
    }
}
