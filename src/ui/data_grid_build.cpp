#include "gran/ui/data_grid.hpp"

#include <utility>

namespace gran::ui
{
    namespace
    {
        // Узел, который строит движок: вид и место в разметке — у таблицы / A node the engine builds: the kind and the markup place are the table's
        std::unique_ptr<Node> makePart(const Node& grid, ElementKind kind)
        {
            std::unique_ptr<Node> node = std::make_unique<Node>();

            node->kind = kind;
            node->line = grid.line;
            node->column = grid.column;
            return node;
        }

        /*
        ** Текст, занимающий всю ширину ячейки и обрезаемый многоточием: длинное значение не раздвигает
        ** столбец и не переносится (перенос запрещает стиль темы ячейки — textWrap наследуется).
        **
        ** A text taking the whole cell width and cut by an ellipsis: a long value does not widen
        ** the column and does not wrap (the cell theme style forbids wrapping — textWrap is inherited).
        */
        std::unique_ptr<Node> makeCellText(const Node& grid, std::string text, const style::PropertySet& cellStyle)
        {
            std::unique_ptr<Node> node = makePart(grid, ElementKind::TEXT);

            node->content.text = std::move(text);
            node->declared.base = cellStyle;
            style::setDeclaration(node->declared.base, style::PropertyId::GROW, 1.0f);
            style::setDeclaration(node->declared.base, style::PropertyId::TEXT_OVERFLOW,
                static_cast<int>(style::TextOverflow::ELLIPSIS));
            return node;
        }

        /*
        ** Заголовок столбца: подпись и место под стрелку сортировки. Щелчок по заголовку сортируемого столбца —
        ** встроенное действие gran:sortColumn (его выполняет приложение через прокси сортировки).
        **
        ** A column header: the title and room for the sort arrow. A click on a sortable column header is
        ** the gran:sortColumn built-in action (the application performs it through the sort proxy).
        ** Подпись выровнена, как значения столбца (textAlign) / The title is aligned as the column values (textAlign)
        */
        std::unique_ptr<Node> makeHeaderCell(const Node& grid, const GridColumnSpec& spec)
        {
            std::unique_ptr<Node> cell = makePart(grid, ElementKind::GRID_HEADER_CELL);
            const style::PropertyValue* align = style::findDeclaration(spec.cellStyle, style::PropertyId::TEXT_ALIGN);
            const GridColumn& column = spec.column;
            style::PropertySet titleStyle;

            if (align != nullptr)
                style::setDeclaration(titleStyle, style::PropertyId::TEXT_ALIGN, *align);
            appendChild(*cell, makeCellText(grid, column.title, titleStyle));
            appendChild(*cell, makePart(grid, ElementKind::TEXT));
            if (column.sortable && !column.field.empty())
                cell->content.onClick = "gran:sortColumn";
            return cell;
        }

        // Шаблон строки: ячейка на столбец — копия <CellTemplate> или текст «{поле}» / The row template: a cell per column — a <CellTemplate> copy or the "{field}" text
        std::unique_ptr<Node> makeRowTemplate(const Node& grid, std::vector<GridColumnSpec>& specs)
        {
            std::unique_ptr<Node> row = makePart(grid, ElementKind::GRID_ROW);

            for (GridColumnSpec& spec : specs)
            {
                Node& cell = appendChild(*row, makePart(grid, ElementKind::GRID_CELL));
                if (spec.cellTemplate)
                    appendChild(cell, std::move(spec.cellTemplate));
                else if (!spec.column.field.empty())
                    appendChild(cell, makeCellText(grid, "{" + spec.column.field + "}", spec.cellStyle));
            }
            return row;
        }
    }

    /*
    ** Порядок детей таблицы: [0] строка заголовков, [1] прокрутка со списком строк — на него опираются
    ** gridHeader и gridRows. Прокрутка растягивается на оставшуюся высоту таблицы (grow), заголовок не сжимается.
    **
    ** The table children order: [0] the header row, [1] the scroll with the row list — gridHeader and gridRows
    ** rely on it. The scroll stretches over the remaining table height (grow), the header does not shrink.
    */
    void buildDataGrid(Node& grid, std::vector<GridColumnSpec>& specs)
    {
        std::shared_ptr<GridState> state = std::make_shared<GridState>();
        std::unique_ptr<Node> header = makePart(grid, ElementKind::GRID_HEADER);
        std::unique_ptr<Node> scroll = makePart(grid, ElementKind::SCROLL);
        std::unique_ptr<Node> rows = makePart(grid, ElementKind::STACK);

        for (const GridColumnSpec& spec : specs)
        {
            state->columns.push_back(spec.column);
            appendChild(*header, makeHeaderCell(grid, spec));
        }
        rows->content.items = std::move(grid.content.items);
        rows->content.selection = std::move(grid.content.selection);
        rows->content.virtualize = grid.content.virtualize;
        rows->itemTemplate = makeRowTemplate(grid, specs);
        grid.content.items.clear();
        grid.content.selection.clear();
        grid.content.grid = std::move(state);
        style::setDeclaration(header->declared.base, style::PropertyId::SHRINK, 0.0f);
        style::setDeclaration(scroll->declared.base, style::PropertyId::GROW, 1.0f);
        appendChild(*scroll, std::move(rows));
        appendChild(grid, std::move(header));
        appendChild(grid, std::move(scroll));
    }
}
