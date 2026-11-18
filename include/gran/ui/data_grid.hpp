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
    **
    ** key — имя столбца в сохранённой раскладке (columnLayout): id столбца, без него — field. resizable — ширину
    ** можно менять мышью и клавиатурой; resized — ширину уже менял пользователь (она уходит в раскладку).
    **
    ** key is the column name in the saved layout (columnLayout): the column id, without it — field. resizable — the width
    ** can be changed by the mouse and the keyboard; resized — the user has already changed the width (it goes into the layout).
    */
    struct GridColumn
    {
        std::string field;
        std::string title;
        std::string key;
        style::GridTrack width;
        float minWidth = 0.0f;
        float maxWidth = std::numeric_limits<float>::infinity();
        bool sortable = true;
        bool resizable = true;
        bool resized = false;
        GridSortType sortType = GridSortType::AUTO;
    };

    // Самый узкий столбец, до которого его сужает пользователь (если minWidth меньше) / The narrowest column the user narrows it to (if minWidth is smaller)
    constexpr float GRID_MIN_RESIZE = 24.0f;

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
    /*
    ** Вид таблицы по горизонтали (пункт 2.2.3): frozen — первые столбцы закреплены и не уезжают при прокрутке;
    ** scrollX — смещение прокрутки, по которому разложены заголовок и закреплённые строки (его ставит компоновка
    ** таблицы до размещения); [shownFirst, shownEnd) — незакреплённые столбцы в окне прокрутки: ячейки остальных
    ** не меряются, не размещаются и не рисуются (виртуализация столбцов). reorderable, resizable — разрешения
    ** пользователю; customized — раскладку менял пользователь или она пришла из columnLayout: тогда она
    ** пишется в разметку и сохраняется в настройках.
    **
    ** The table horizontal view (item 2.2.3): frozen — the first columns are frozen and do not move away on scrolling;
    ** scrollX is the scroll offset the header and the pinned rows are laid out by (the table layout sets it
    ** before placement); [shownFirst, shownEnd) are the non-frozen columns in the scroll viewport: the cells of the others
    ** are not measured, placed or painted (column virtualization). reorderable, resizable are the user
    ** permissions; customized — the user changed the layout or it came from columnLayout: then it is
    ** written into the markup and kept in the settings.
    */
    struct GridView
    {
        std::size_t frozen = 0;
        float scrollX = 0.0f;
        std::size_t shownFirst = 0;
        std::size_t shownEnd = NO_COLUMN;
        bool reorderable = true;
        bool resizable = true;
        bool customized = false;
    };

    struct GridState
    {
        std::vector<GridColumn> columns;
        std::vector<float> widths;
        std::vector<float> autoWidths;
        std::size_t current = 0;
        std::size_t sorted = NO_COLUMN;
        bool descending = false;
        GridView view;
    };

    /*
    ** Настройки таблицы из разметки: закреплённые столбцы, разрешения, сохранённая раскладка столбцов
    ** (columnLayout: «ключ[:ширина] …» — порядок и изменённые ширины) и модели закреплённых строк сверху и снизу.
    **
    ** The table settings from the markup: the frozen columns, the permissions, the saved column layout
    ** (columnLayout: "key[:width] …" — the order and the changed widths) and the pinned row models at the top and the bottom.
    */
    struct GridOptions
    {
        std::size_t frozen = 0;
        bool reorderable = true;
        bool resizable = true;
        std::string layout;
        std::string pinnedTop;
        std::string pinnedBottom;
    };

    // ---- data_grid_build.cpp ----

    /*
    ** Части таблицы по столбцам: строка заголовков, прокрутка и список строк. items, selection и virtual
    ** таблицы переходят к списку строк; шаблон строки — ячейка на столбец (поле или копия <CellTemplate>).
    **
    ** The table parts by the columns: the header row, the scroll and the row list. The table items, selection and virtual
    ** go to the row list; the row template is a cell per column (the field or a <CellTemplate> copy).
    */
    void buildDataGrid(Node& grid, std::vector<GridColumnSpec>& specs, const GridOptions& options);

    // ---- data_grid_pinned.cpp ----

    // Настройки вида из разметки и закреплённые списки строк над и под прокруткой (часть buildDataGrid) / The view settings from the markup and the pinned row lists above and below the scroll (a part of buildDataGrid)
    void finishDataGrid(Node& grid, const GridOptions& options);

    // ---- data_grid_query.cpp ----

    // Ближайшая таблица среди предков узла (сам узел не в счёт); nullptr — узел не из таблицы / The nearest table among the node ancestors (the node itself does not count); nullptr — the node is not from a table
    Node* gridOf(const Node& node) noexcept;

    // Строка заголовков и список строк таблицы (nullptr — не таблица или части не построены) / The table header row and row list (nullptr — not a table or the parts are not built)
    Node* gridHeader(Node& grid) noexcept;
    Node* gridRows(Node& grid) noexcept;

    // Номер ячейки строки row, внутри которой лежит узел hit; NO_COLUMN — узел не в ячейке строки / The index of the row cell holding the hit node; NO_COLUMN — the node is not in a row cell
    std::size_t cellColumn(const Node& row, const Node& hit) noexcept;

    // ---- data_grid_lines.cpp ----

    // Строки, раскладываемые по столбцам: заголовок, закреплённые строки, копии списка строк / The lines laid out by the columns: the header, the pinned rows, the row list copies
    std::vector<Node*> gridLines(Node& grid);

    // Строка таблицы прокручивается по вертикали (копия списка строк), а не стоит над или под прокруткой / The table line scrolls vertically (a row list copy) rather than stands above or below the scroll
    bool scrolledLine(const Node& line) noexcept;

    // Закреплённый список стоит над прокруткой (pinnedTop), а не под ней / The pinned list stands above the scroll (pinnedTop) rather than below it
    bool pinnedAbove(const Node& list) noexcept;

    // ---- data_grid_columns.cpp ----

    // Переставить столбец from на место to (заголовки, шаблоны и копии строк, ширины); true — порядок сменился / Move the column from to the place to (the headers, the templates and the row copies, the widths); true — the order changed
    bool moveGridColumn(Node& grid, std::size_t from, std::size_t to);

    // ---- data_grid_resize.cpp ----

    // Новая ширина столбца от пользователя (px или auto; пределы — minWidth/maxWidth); true — сменилась / A new column width from the user (px or auto; the limits — minWidth/maxWidth); true — it changed
    bool setGridColumnWidth(Node& grid, std::size_t column, const style::GridTrack& width);

    // Пиксельная ширина от пользователя в пределах столбца: не уже max(minWidth, GRID_MIN_RESIZE), не шире maxWidth / A pixel width from the user within the column: not narrower than max(minWidth, GRID_MIN_RESIZE), not wider than maxWidth
    float limitColumnWidth(const GridColumn& column, float width) noexcept;

    // ---- data_grid_layout_text.cpp ----

    // Раскладка для columnLayout: ключи столбцов по порядку, у изменённых пользователем — «:ширина» / The layout for columnLayout: the column keys in order, the user-changed ones with ":width"
    std::string gridLayoutText(const GridState& state);

    /*
    ** Сохранённая раскладка → столбцы до построения: известные ключи встают по порядку и получают ширину, неизвестные
    ** (столбец убран из новой версии программы) и испорченные ширины пропускаются, остальные столбцы идут следом
    ** в порядке разметки. false — раскладка пуста.
    **
    ** A saved layout → the columns before building: the known keys stand in order and get the width, the unknown ones
    ** (a column removed from a new program version) and the damaged widths are skipped, the other columns follow
    ** in the markup order. false — the layout is empty.
    */
    bool applyGridLayout(std::vector<GridColumnSpec>& specs, const std::string& text);

    // ---- data_grid_view.cpp ----

    // Ячейка index строки line видна (закреплена или в окне прокрутки); вне таблицы — всегда / The cell index of the line is shown (frozen or in the scroll viewport); outside a table — always
    bool gridCellShown(const Node& line, std::size_t index) noexcept;

    // Закреплённых ячеек у строки line (0 — не строка таблицы) / The frozen cells of the line (0 — not a table line)
    std::size_t gridFrozen(const Node& line) noexcept;

    // Правый край закреплённых ячеек строки в координатах окна: левее него незакреплённые не видны / The right edge of the line frozen cells in window coordinates: the non-frozen ones are not visible to its left
    float gridFrozenEdge(const Node& line) noexcept;

    // Ячейка видна в точке с абсциссой x (координаты компоновки): показана и не ушла под закреплённые / The cell is visible at the abscissa x (layout coordinates): shown and not gone under the frozen ones
    bool gridCellVisibleAt(const Node& line, std::size_t index, float x) noexcept;

    // ---- data_grid_reveal.cpp ----

    // Прокрутить таблицу по горизонтали, чтобы текущий столбец был виден целиком; true — смещение сменилось / Scroll the table horizontally so the current column is fully visible; true — the offset changed
    bool revealGridColumn(Node& grid);

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
