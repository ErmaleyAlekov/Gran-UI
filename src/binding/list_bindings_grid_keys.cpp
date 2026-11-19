#include "gran/binding/internal/list_binding.hpp"
#include "gran/ui/data_grid.hpp"

namespace gran::binding
{
    namespace
    {
        /*
        ** Таблица над плоским деревом (TreeList): в столбце-дереве (tree, без него — первый) стрелки — дереву, как в проводнике: влево — свернуть
        ** или к родителю, вправо — раскрыть свёрнутую ветку; у раскрытой ветки и листа вправо идёт к следующему столбцу.
        ** Раньше таблица забирала влево/вправо целиком, и дерево в таблице не раскрывалось с клавиатуры (ревью Т-63).
        **
        ** A table over a flat tree (TreeList): in the tree column (tree, without one — the first) the arrows go to the tree, as in a file manager: left — collapse
        ** or go to the parent, right — expand a collapsed branch; for an expanded branch and a leaf right goes to the next column.
        ** Previously the table took left/right entirely, and the tree in a table did not expand from the keyboard (review T-63).
        */
        bool treeOwnsKey(ListBinding& binding, ListKey key, const ui::GridState& state)
        {
            data::TreeList* tree = dynamic_cast<data::TreeList*>(&binding.model());
            std::size_t current = data::NO_ROW;

            if (tree == nullptr || state.current != ui::gridTreeColumn(state) || (key != ListKey::LEFT && key != ListKey::RIGHT))
                return false;
            if (key == ListKey::LEFT)
                return true;
            if (binding.selection() != nullptr)
                current = binding.selection()->current();
            return current < tree->size() && tree->hasChildren(current) && !tree->expanded(current);
        }
    }

    /*
    ** Клавиши текущего столбца: влево/вправо — соседний, Home/End без Ctrl — первый/последний (с Ctrl — первая
    ** и последняя строка, как в электронных таблицах). Отметки строк не меняются, текущая ячейка перерисовывается.
    **
    ** The current column keys: left/right — the neighbor, Home/End without Ctrl — the first/last (with Ctrl — the first
    ** and the last row, as in spreadsheets). The row marks do not change, the current cell is repainted.
    */
    bool gridKey(ListBinding& binding, ListKey key, bool control)
    {
        ui::Node* grid = ui::gridOf(binding.container());
        std::size_t column = 0;

        if (grid == nullptr || (control && (key == ListKey::HOME || key == ListKey::END)))
            return false;
        column = grid->content.grid->current;
        if (treeOwnsKey(binding, key, *grid->content.grid))
            return false;
        if (key == ListKey::LEFT && column > 0)
            --column;
        else if (key == ListKey::RIGHT)
            ++column;
        else if (key == ListKey::HOME)
            column = 0;
        else if (key == ListKey::END)
            column = ui::NO_COLUMN;
        else if (key != ListKey::LEFT)
            return false;
        if (ui::setGridColumn(*grid, column))
            binding.selectionChanged();
        ui::revealGridColumn(*grid);
        return true;
    }
}
