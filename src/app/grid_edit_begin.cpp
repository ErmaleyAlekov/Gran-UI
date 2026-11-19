#include "gran/app/internal/app_state.hpp"

#include <utility>

namespace gran::app
{
    namespace
    {
        // Тип значений столбца: явный type или тип <Column> источника, иначе текст / The column value type: an explicit type or the source <Column> type, otherwise text
        data::ColumnType editTypeOf(const ui::GridColumn& column, const data::ListSource& source)
        {
            if (column.editType == ui::GridEditType::TEXT)
                return data::ColumnType::TEXT;
            if (column.editType == ui::GridEditType::NUMBER)
                return data::ColumnType::NUMBER;
            if (column.editType == ui::GridEditType::DATE)
                return data::ColumnType::DATE;
            if (column.editType == ui::GridEditType::BOOLEAN)
                return data::ColumnType::BOOLEAN;
            for (const data::Column& described : source.columns())
            {
                if (described.name == column.field)
                    return described.type;
            }
            return data::ColumnType::TEXT;
        }

        // Копия строки записи index среди детей списка (nullptr — запись вне окна виртуального списка) / The row copy of the record index among the list children (nullptr — the record is outside the virtual list window)
        ui::Node* rowCopy(const binding::ListBindings& lists, ui::Node& rows, const data::ListSource& source,
            std::size_t index)
        {
            for (const std::unique_ptr<ui::Node>& child : rows.children)
            {
                if (child->kind != ui::ElementKind::GRID_ROW)
                    continue;
                binding::ItemRef item = lists.itemOf(*child);
                if (item.model == &source && item.index == index)
                    return child.get();
            }
            return nullptr;
        }

        /*
        ** Содержимое ячейки прячется, а не удаляется: оно привязано к записи (подстановки {поле}) и после правки
        ** показывает уже новое значение. Прячутся только видимые дети — их и вернёт closeEditor.
        **
        ** The cell content is hidden rather than removed: it is bound to the record ({field} placeholders) and after the edit
        ** shows the new value already. Only the visible children are hidden — exactly those closeEditor brings back.
        */
        void hideCellContent(ui::Node& cell, std::vector<ui::Node*>& hidden)
        {
            for (const std::unique_ptr<ui::Node>& child : cell.children)
            {
                // Кнопка раскрытия столбца-дерева остаётся: редактор встаёт на место текста, а не на место отступа
                // The tree column expander button stays: the editor takes the place of the text, not of the indent
                if (!child->visible || child->kind == ui::ElementKind::TREE_EXPANDER)
                    continue;
                child->visible = false;
                hidden.push_back(child.get());
            }
            ui::markDirty(cell, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        }

        /*
        ** Сеанс правки заводится до фокуса: смена фокуса сообщает окну (gridEditFocusChanged), и фокус в редакторе
        ** не должен выглядеть как уход из него. Набранный символ заменяет значение (каретка за ним), иначе
        ** значение выделено целиком — как в электронных таблицах.
        **
        ** The edit session is created before the focus: a focus change notifies the window (gridEditFocusChanged), and the focus
        ** in the editor must not look like leaving it. A typed character replaces the value (the caret after it), otherwise
        ** the value is selected whole — as in spreadsheets.
        */
        bool openEditor(AppWindow& win, const GridEditPlace& place, const ui::GridColumn& column, data::ColumnType type,
            const std::string* typed)
        {
            const data::Record& record = place.source->at(place.index);
            std::unique_ptr<ui::Node> made = makeGridEditor(*place.cell, column, type, record);
            ui::Node* value = editorValueNode(*made);
            if (value == nullptr || !win.router)
                return false;
            std::unique_ptr<GridEdit> edit = makeGridSession(place, column, type);
            hideCellContent(*place.cell, edit->hidden);
            edit->editor = &ui::appendChild(*place.cell, std::move(made));
            edit->value = value;
            ui::markDirty(*edit->editor, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            win.gridEdit = std::move(edit);
            win.shared.lists.bind(*win.gridEdit->editor);
            if (typed != nullptr)
                writeEditorValue(*value, *typed, false);
            else
                writeEditorValue(*value, win.gridEdit->original, true);
            win.router->focus(value);
            return true;
        }
    }

    /*
    ** Открытая правка здесь не закрывается (ревью Т-69): вызов идёт из середины работы маршрутизатора ввода, а запись
    ** в модель перестраивает строки. Отложенную правку закрывает безопасная точка до события (handleEvent), поэтому
    ** сюда приходит только правка с фокусом в редакторе — тогда новая не начинается. Текущая строка — текущая запись модели выделения, текущий
    ** столбец — у таблицы; запись должна быть в окне виртуального списка (её показывают стрелки и щелчок).
    **
    ** An open edit is not closed here (review T-69): the call comes from the middle of the input router work, and writing
    ** into the model rebuilds the rows. A deferred edit is closed by the safe point before the event (handleEvent), so
    ** only an edit with the focus in the editor reaches here — then a new one does not start. The current row is the selection model current record, the current
    ** column is the table one; the record must be in the virtual list window (the arrows and the click show it).
    */
    bool beginGridEdit(AppWindow& win, ui::Node& rows, const std::string* typed)
    {
        ui::Node* grid = ui::gridOf(rows);

        if (grid == nullptr || ui::gridRows(*grid) != &rows || !grid->content.grid->editable)
            return false;
        if (win.gridEdit)
            return false;
        data::SelectionModel* selection = win.shared.lists.selection(rows.content.selection);
        data::ListSource* source = win.shared.lists.list(rows.content.items);
        ui::GridState& state = *grid->content.grid;
        if (selection == nullptr || source == nullptr || &selection->source() != source
            || selection->current() >= source->size() || !ui::gridColumnEditable(state, state.current))
            return false;
        ui::Node* row = rowCopy(win.shared.lists, rows, *source, selection->current());
        ui::Node* cell = nullptr;
        if (row != nullptr)
            cell = ui::gridCell(*row, state.current);
        data::ColumnType type = editTypeOf(state.columns[state.current], *source);
        if (cell == nullptr || (typed != nullptr && type == data::ColumnType::BOOLEAN))
            return false;
        GridEditPlace place{grid, &rows, cell, source, selection->current()};
        return openEditor(win, place, state.columns[state.current], type, typed);
    }
}
