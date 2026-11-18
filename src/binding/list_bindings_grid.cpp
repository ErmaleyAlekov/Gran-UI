#include "gran/binding/internal/list_binding.hpp"
#include "gran/ui/data_grid.hpp"

namespace gran::binding
{
    namespace
    {
        // Способ сравнения столбца: явный sortType или тип <Column> источника (число — как число) / The column comparison: an explicit sortType or the source <Column> type (a number — as a number)
        data::SortType sortTypeOf(const ui::GridColumn& column, const data::ListSource& source)
        {
            if (column.sortType == ui::GridSortType::TEXT)
                return data::SortType::TEXT;
            if (column.sortType == ui::GridSortType::NUMBER)
                return data::SortType::NUMBER;
            if (column.sortType == ui::GridSortType::NATURAL)
                return data::SortType::NATURAL;
            for (const data::Column& described : source.columns())
            {
                if (described.name == column.field && described.type == data::ColumnType::NUMBER)
                    return data::SortType::NUMBER;
            }
            return data::SortType::TEXT;
        }
    }

    /*
    ** Закреплённые строки — свои списки над и под прокруткой. Если их модель — прокси <SortFilter>, щелчок по заголовку
    ** сортирует и их тем же ключом: иначе закреплённые строки выглядят «сломанными» (ручная проверка Т-64).
    ** Хранилище без прокси остаётся в своём порядке (итоги и т. п. сортировать нечего).
    **
    ** The pinned rows are lists of their own above and below the scroll. If their model is a <SortFilter> proxy, a header click
    ** sorts them by the same key too: otherwise the pinned rows look "broken" (the manual check T-64).
    ** A storage without a proxy keeps its own order (totals and the like have nothing to sort).
    */
    static void sortPinned(const ListBindings& bindings, const ui::Node& grid, const data::SortKeys& keys)
    {
        for (const std::unique_ptr<ui::Node>& child : grid.children)
        {
            data::SortFilterProxy* proxy = nullptr;
            if (child->kind == ui::ElementKind::STACK && !child->content.items.empty())
                proxy = dynamic_cast<data::SortFilterProxy*>(bindings.list(child->content.items));
            if (proxy != nullptr)
                proxy->setSort(keys);
        }
    }

    // Нажатие в ячейку строки делает её столбец текущим (сама строка выделяется как запись списка) / A press in a row cell makes its column current (the row itself is selected as a list record)
    void gridPress(ListBinding& binding, const ui::Node& row, const ui::Node& hit)
    {
        ui::Node* grid = ui::gridOf(binding.container());
        std::size_t column = ui::cellColumn(row, hit);

        if (grid == nullptr || column == ui::NO_COLUMN)
            return;
        if (ui::setGridColumn(*grid, column))
            binding.selectionChanged();
        ui::revealGridColumn(*grid);
    }

    // Стрелка в заголовке по первому ключу прокси, если он — поле одного из столбцов / The header arrow by the first proxy key if it is the field of one of the columns
    void syncGridSort(const ui::Node& rows, const data::ListSource& model)
    {
        const data::SortFilterProxy* proxy = dynamic_cast<const data::SortFilterProxy*>(&model);
        ui::Node* grid = ui::gridOf(rows);

        // Закреплённые строки — свой список со своей моделью: стрелку в заголовке ставит только основной список
        // The pinned rows are a list of their own with their own model: only the main list sets the header arrow
        if (grid == nullptr || proxy == nullptr || proxy->sort().empty() || ui::gridRows(*grid) != &rows)
            return;
        const data::SortKey& key = proxy->sort().front();
        const std::vector<ui::GridColumn>& columns = grid->content.grid->columns;
        for (std::size_t index = 0; index < columns.size(); ++index)
        {
            if (columns[index].field == key.field)
            {
                ui::markSortedColumn(*grid, index, key.descending);
                return;
            }
        }
    }

    /*
    ** Первый щелчок сортирует по возрастанию, повторный по тому же столбцу — меняет направление. Сортирует
    ** прокси <SortFilter>, на который смотрит таблица: копии строк переставляются, выделение идёт за записями.
    **
    ** The first click sorts ascending, a repeated one on the same column reverses the direction. The <SortFilter> proxy
    ** the table looks at sorts: the row copies are rearranged, the selection follows the records.
    */
    bool ListBindings::sortColumn(const ui::Node& headerCell)
    {
        ui::Node* grid = ui::gridOf(headerCell);
        data::SortFilterProxy* proxy = nullptr;

        if (grid == nullptr || ui::gridRows(*grid) == nullptr || ui::gridHeader(*grid) == nullptr)
            return false;
        ListBinding* binding = bindingOf(*ui::gridRows(*grid));
        if (binding != nullptr)
            proxy = dynamic_cast<data::SortFilterProxy*>(&binding->model());
        std::size_t column = ui::cellColumn(*ui::gridHeader(*grid), headerCell);
        ui::GridState& state = *grid->content.grid;
        if (proxy == nullptr || column >= state.columns.size() || !state.columns[column].sortable
            || state.columns[column].field.empty())
            return false;
        bool descending = state.sorted == column && !state.descending;
        data::SortKeys keys{data::SortKey{state.columns[column].field, sortTypeOf(state.columns[column], *proxy), descending}};
        // Стрелка — до сортировки: наблюдатели прокси (код программы) могут перестроить дерево, и узел таблицы
        // после setSort уже не трогается (ревью Т-63) / The arrow goes before sorting: the proxy observers (program code)
        // may rebuild the tree, and the table node is not touched after setSort (review T-63)
        ui::markSortedColumn(*grid, column, descending);
        sortPinned(*this, *grid, keys);
        proxy->setSort(std::move(keys));
        return true;
    }
}
