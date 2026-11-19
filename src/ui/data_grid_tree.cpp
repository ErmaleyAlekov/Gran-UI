#include "gran/ui/data_grid.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/property.hpp"

namespace gran::ui
{
    namespace
    {
        // Узел, который строит движок: место в разметке — у таблицы / A node the engine builds: the markup place is the table's
        std::unique_ptr<Node> makePart(const Node& grid, ElementKind kind)
        {
            std::unique_ptr<Node> node = std::make_unique<Node>();

            node->kind = kind;
            node->line = grid.line;
            node->column = grid.column;
            return node;
        }

        /*
        ** Поле фильтра: нулевая своя ширина и рост (как у редактора ячейки) — поле не раздвигает столбец auto,
        ** а занимает ширину, которую дали столбцу. Подсказка и имя для экранного диктора — из каталога и заголовка.
        **
        ** A filter input: a zero own width and growth (as for a cell editor) — the input does not widen an auto column
        ** but takes the width given to the column. The hint and the screen reader name come from the catalog and the title.
        */
        std::unique_ptr<Node> makeFilterInput(const Node& grid, const GridColumn& column)
        {
            std::unique_ptr<Node> input = makePart(grid, ElementKind::INPUT);

            input->content.onChange = "gran:filterColumn";
            input->content.placeholder = i18n::format(i18n::Message::GRID_FILTER_PLACEHOLDER);
            input->content.accessibleName = column.title;
            input->content.text = column.filterText;
            style::setDeclaration(input->declared.base, style::PropertyId::WIDTH, style::Length{0.0f, style::LengthUnit::PIXELS});
            style::setDeclaration(input->declared.base, style::PropertyId::GROW, 1.0f);
            return input;
        }
    }

    std::size_t gridTreeColumn(const GridState& state) noexcept
    {
        for (std::size_t index = 0; index < state.columns.size(); ++index)
        {
            if (state.columns[index].tree)
                return index;
        }
        return 0;
    }

    std::unique_ptr<Node> makeGridFilterRow(const Node& grid, const std::vector<GridColumn>& columns)
    {
        std::unique_ptr<Node> row = makePart(grid, ElementKind::GRID_HEADER);

        for (const GridColumn& column : columns)
        {
            Node& cell = appendChild(*row, makePart(grid, ElementKind::GRID_CELL));
            if (column.filterable && !column.field.empty())
                appendChild(cell, makeFilterInput(grid, column));
        }
        style::setDeclaration(row->declared.base, style::PropertyId::SHRINK, 0.0f);
        return row;
    }

    void readGridFilterTexts(Node& grid) noexcept
    {
        Node* line = gridFilterRow(grid);

        if (line == nullptr || !grid.content.grid)
            return;
        std::vector<GridColumn>& columns = grid.content.grid->columns;
        for (std::size_t index = 0; index < columns.size() && index < line->children.size(); ++index)
        {
            const Node& cell = *line->children[index];
            if (!cell.children.empty() && cell.children.front()->kind == ElementKind::INPUT)
                columns[index].filterText = cell.children.front()->content.text;
        }
    }
}
