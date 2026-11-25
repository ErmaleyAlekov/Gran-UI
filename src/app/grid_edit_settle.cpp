#include "gran/app/internal/app_state.hpp"
#include "gran/ui/internal/preorder.hpp"

namespace gran::app
{
    namespace
    {
        // Номер столбца правки по ключу (столбцы могли переставить во время правки); NO_COLUMN — нет такого / The edited column index by the key (the columns might be reordered during the edit); NO_COLUMN — none
        std::size_t editedColumn(const ui::Node& grid, const std::string& key) noexcept
        {
            const auto& columns = grid.content.grid->columns;
            for (std::size_t index = 0; index < columns.size(); ++index)
            {
                if (columns[index].key == key)
                    return index;
            }
            return ui::NO_COLUMN;
        }

        // Идентичность узла сохраняется при перемещении / A node keeps its identity when moved
        ui::Node* liveNode(ui::Node* root, std::uint64_t serial)
        {
            if (!root || !serial) return nullptr;
            ui::internal::Preorder traversal(*root);
            while (auto* node = traversal.next())
                if (node->serial == serial) return node;
            return nullptr;
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
        const auto gridSerial = edit.grid ? edit.grid->serial : 0;
        const auto rowsSerial = edit.rows ? edit.rows->serial : 0;
        const auto key = edit.key;
        const auto record = edit.record;
        edit.pending = GridEditEnd::NONE;
        edit.tab = GridTab::NONE;
        if (finishGridEdit(win, pending != GridEditEnd::CANCEL))
        {
            if (pending != GridEditEnd::SUBMIT || tab == GridTab::NONE || win.gridEdit || !record || !record->alive())
                return true;
            auto* grid = liveNode(win.document.root.get(), gridSerial);
            auto* rows = liveNode(grid, rowsSerial);
            if (!rows || !grid->content.grid) return true;
            const auto* selection = win.shared.lists.selection(rows->content.selection);
            if (!selection || &selection->source() != &record->source() || selection->current() != record->index())
                return true;
            const auto column = editedColumn(*grid, key);
            if (column != ui::NO_COLUMN)
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
