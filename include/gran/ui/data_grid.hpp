#ifndef GRAN_UI_DATA_GRID_HPP
#define GRAN_UI_DATA_GRID_HPP

#include "gran/style/property.hpp"
#include "gran/style/values.hpp"
#include "gran/ui/node.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace gran::ui
{
    // Способ сравнения при сортировке по столбцу; AUTO — по типу столбца источника (<Column type>)
    // The comparison for sorting by a column; AUTO — by the source column type (<Column type>)
    enum class GridSortType : std::uint8_t { AUTO, TEXT, NUMBER, NATURAL };

    // «Нет столбца» (таблица не отсортирована) / "No column" (the table is not sorted)
    constexpr std::size_t NO_COLUMN = static_cast<std::size_t>(-1);

    /*
    ** Столбец таблицы <GridColumn>: поле записи (ячейка по умолчанию и ключ сортировки), заголовок,
    ** ширина как дорожка сетки (px, auto, fr, minmax) и её пределы, можно ли сортировать и как сравнивать.
    **
    ** A <GridColumn> table column: the record field (the default cell and the sort key), the title,
    ** the width as a grid track (px, auto, fr, minmax) and its limits, whether it can be sorted and how to compare.
    */
    struct GridColumn
    {
        std::string field;
        std::string title;
        style::GridTrack width;
        float minWidth = 0.0f;
        float maxWidth = std::numeric_limits<float>::infinity();
        bool sortable = true;
        GridSortType sortType = GridSortType::AUTO;
    };

    /*
    ** Описание столбца для построения: сам столбец, свойства стиля для текста ячеек поля (textAlign, color…)
    ** и шаблон ячейки <CellTemplate> (nullptr — ячейка показывает поле field).
    **
    ** A column description for building: the column itself, the style properties for the field cell text (textAlign, color…)
    ** and the <CellTemplate> cell template (nullptr — the cell shows the field).
    */
    struct GridColumnSpec
    {
        GridColumn column;
        style::PropertySet cellStyle;
        std::unique_ptr<Node> cellTemplate;
    };

    /*
    ** Состояние таблицы (NodeContent::grid узла DataGrid). widths — ширины столбцов последней компоновки,
    ** autoWidths — наибольшее содержимое столбца, виденное до сих пор (при прокрутке виртуальной таблицы
    ** столбец auto не сужается — строки не «прыгают»), current — текущий столбец (клавиши влево/вправо),
    ** sorted и descending — столбец, по которому отсортировано, и направление (стрелка в заголовке).
    **
    ** The table state (NodeContent::grid of a DataGrid node). widths are the column widths of the last layout,
    ** autoWidths is the largest column content seen so far (while scrolling a virtual table an auto column
    ** does not narrow — the rows do not "jump"), current is the current column (the left/right keys),
    ** sorted and descending are the column the table is sorted by and the direction (the arrow in the header).
    */
    struct GridState
    {
        std::vector<GridColumn> columns;
        std::vector<float> widths;
        std::vector<float> autoWidths;
        std::size_t current = 0;
        std::size_t sorted = NO_COLUMN;
        bool descending = false;
    };

    // ---- data_grid_build.cpp ----

    /*
    ** Части таблицы по столбцам: строка заголовков, прокрутка и список строк. items, selection и virtual
    ** таблицы переходят к списку строк; шаблон строки — ячейка на столбец (поле или копия <CellTemplate>).
    **
    ** The table parts by the columns: the header row, the scroll and the row list. The table items, selection and virtual
    ** go to the row list; the row template is a cell per column (the field or a <CellTemplate> copy).
    */
    void buildDataGrid(Node& grid, std::vector<GridColumnSpec>& specs);

    // ---- data_grid_query.cpp ----

    // Ближайшая таблица среди предков узла (сам узел не в счёт); nullptr — узел не из таблицы / The nearest table among the node ancestors (the node itself does not count); nullptr — the node is not from a table
    Node* gridOf(const Node& node) noexcept;

    // Строка заголовков и список строк таблицы (nullptr — не таблица или части не построены) / The table header row and row list (nullptr — not a table or the parts are not built)
    Node* gridHeader(Node& grid) noexcept;
    Node* gridRows(Node& grid) noexcept;

    // Номер ячейки строки row, внутри которой лежит узел hit; NO_COLUMN — узел не в ячейке строки / The index of the row cell holding the hit node; NO_COLUMN — the node is not in a row cell
    std::size_t cellColumn(const Node& row, const Node& hit) noexcept;

    // ---- data_grid_marks.cpp ----

    // Состояние CURRENT ячейке текущего столбца, если строка текущая; остальным ячейкам — снять / The CURRENT state for the current column cell if the row is current; drop it from the other cells
    void markCurrentCell(Node& row, bool currentRow);

    // Сделать столбец текущим (за пределами — ближайший край); true — текущий столбец сменился / Make the column current (beyond the bounds — the nearest edge); true — the current column changed
    bool setGridColumn(Node& grid, std::size_t column);

    // Стрелка сортировки и состояние CHECKED у заголовка столбца column (NO_COLUMN — ни у кого) / The sort arrow and the CHECKED state at the column header (NO_COLUMN — at none)
    void markSortedColumn(Node& grid, std::size_t column, bool descending);

    // Записи списка строк удалены или изменены: ширины столбцов auto копятся заново (rows — список строк таблицы) / The row list records were removed or changed: the auto column widths accumulate anew (rows is the table row list)
    void forgetGridContent(const Node& rows) noexcept;
}

#endif
