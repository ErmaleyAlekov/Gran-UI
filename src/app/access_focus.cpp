#include "gran/app/internal/access_items.hpp"
#include "gran/app/internal/app_state.hpp"
#include "gran/ui/data_grid.hpp"

namespace gran::app
{
    /*
    ** Текущая запись есть, но её копии нет (виртуальный список прокручен далеко) — фокус остаётся у списка:
    ** провайдер ОС не может создать узел строки посреди своего вызова. Текущий столбец таблицы — тот, по которому
    ** ходят стрелки влево и вправо.
    **
    ** The current record exists but has no copy (the virtual list is scrolled far away) — the focus stays at the list:
    ** the OS provider cannot create a row node in the middle of its call. The current table column is the one
    ** the left and right arrows walk over.
    */
    const ui::Node* accessFocusOf(const AppWindow& win, const ui::Node* focus) noexcept
    {
        if (focus == nullptr)
            return focus;
        binding::ItemRef list = win.shared.lists.listOf(*focus);
        if (list.selection == nullptr || list.index == data::NO_ROW)
            return focus;
        ui::Node* item = win.shared.lists.itemNode(*focus, list.index);
        if (item == nullptr || !item->visible)
            return focus;
        const ui::Node* grid = ui::gridOf(*item);
        if (item->kind != ui::ElementKind::GRID_ROW || grid == nullptr || !grid->content.grid)
            return item;
        ui::Node* cell = ui::gridCell(*item, grid->content.grid->current);
        if (cell == nullptr || !cell->visible)
            return item;
        return cell;
    }

    bool AppWindow::listReveal(ui::Node& container, std::size_t index)
    {
        return shared.lists.revealItem(container, index);
    }
}
