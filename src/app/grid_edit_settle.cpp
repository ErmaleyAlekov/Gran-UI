#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    namespace
    {
        // Номер столбца правки по ключу (столбцы могли переставить во время правки); NO_COLUMN — нет такого / The edited column index by the key (the columns might be reordered during the edit); NO_COLUMN — none
        std::size_t editedColumn(const GridEdit& edit) noexcept
        {
            if (edit.grid == nullptr)
                return ui::NO_COLUMN;
            const std::vector<ui::GridColumn>& columns = edit.grid->content.grid->columns;
            for (std::size_t index = 0; index < columns.size(); ++index)
            {
                if (columns[index].key == edit.key)
                    return index;
            }
            return ui::NO_COLUMN;
        }

        /*
        ** После записи по Tab — редактор следующего правимого столбца той же строки (Shift+Tab — предыдущего).
        ** Текущий столбец переводится напрямую (moveGridCurrent), а не стрелками: в таблице-дереве стрелка вправо
        ** раскрыла бы ветку. У края строки правка просто заканчивается.
        **
        ** After writing by Tab — the editor of the next editable column of the same row (Shift+Tab — the previous one).
        ** The current column is set directly (moveGridCurrent), not by the arrows: in a tree table the right arrow
        ** would expand a branch. At the row edge the edit just ends.
        */
        void tabOnward(AppWindow& win, ui::Node& grid, ui::Node& rows, std::size_t column, bool backward)
        {
            std::size_t next = ui::nextEditableColumn(*grid.content.grid, column, backward);

            if (next == ui::NO_COLUMN)
                return;
            ui::moveGridCurrent(grid, next);
            beginGridEdit(win, rows, nullptr);
        }
    }

    /*
    ** Безопасная точка — вне работы маршрутизатора ввода: удаление редактора и запись в модель (перестройка строк
    ** наблюдателями) не застают его указатели на полпути. SUBMIT с ошибкой проверки оставляет редактор открытым
    ** (Invalid), COMMIT с ошибкой отменяет правку: фокус уже ушёл, держать редактор незачем.
    **
    ** The safe point is outside the input router work: removing the editor and writing into the model (the observers rebuild
    ** the rows) do not catch its pointers halfway. SUBMIT with a check error keeps the editor open (Invalid),
    ** COMMIT with an error cancels the edit: the focus has already left, there is no point in keeping the editor.
    */
    bool settleGridEdit(AppWindow& win)
    {
        if (!win.gridEdit || win.gridEdit->pending == GridEditEnd::NONE)
            return false;
        GridEdit& edit = *win.gridEdit;
        GridEditEnd pending = edit.pending;
        GridTab tab = edit.tab;
        ui::Node* grid = edit.grid;
        ui::Node* rows = edit.rows;
        std::size_t column = editedColumn(edit);
        edit.pending = GridEditEnd::NONE;
        edit.tab = GridTab::NONE;
        if (finishGridEdit(win, pending != GridEditEnd::CANCEL))
        {
            if (tab != GridTab::NONE && grid != nullptr && rows != nullptr && column != ui::NO_COLUMN)
                tabOnward(win, *grid, *rows, column, tab == GridTab::BACKWARD);
            return true;
        }
        if (pending == GridEditEnd::COMMIT)
            finishGridEdit(win, false);
        return true;
    }

    // Сразу, а не через pending: событие закрытия идёт мимо маршрутизатора ввода / At once rather than through pending: the close event bypasses the input router
    void commitGridEdit(AppWindow& win)
    {
        if (!win.gridEdit)
            return;
        win.gridEdit->pending = GridEditEnd::COMMIT;
        win.gridEdit->tab = GridTab::NONE;
        settleGridEdit(win);
    }
}
