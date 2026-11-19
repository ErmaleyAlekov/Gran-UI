#include "gran/ui/data_grid.hpp"

namespace gran::ui
{
    namespace
    {
        /*
        ** Закреплённый список строк: те же ячейки, что у основного (копия шаблона строки), все записи своей модели
        ** (не виртуальный: закреплённых строк немного — итоги, избранные записи), без выделения. Не сжимается:
        ** место под строки отдаёт прокрутка, как у заголовка.
        **
        ** A pinned row list: the same cells as the main one (a copy of the row template), all records of its model
        ** (not virtual: there are few pinned rows — totals, favorite records), without a selection. It does not shrink:
        ** the scroll gives up the room for the rows, as for the header.
        */
        std::unique_ptr<Node> makePinned(const Node& grid, const Node& rowTemplate, const std::string& items)
        {
            std::unique_ptr<Node> list = std::make_unique<Node>();

            list->kind = ElementKind::STACK;
            list->line = grid.line;
            list->column = grid.column;
            list->content.items = items;
            list->itemTemplate = cloneTree(rowTemplate);
            style::setDeclaration(list->declared.base, style::PropertyId::SHRINK, 0.0f);
            return list;
        }

        // Место прокрутки среди детей таблицы (её точно нашёл gridRows) / The scroll place among the table children (gridRows has surely found it)
        std::size_t scrollIndex(const Node& grid) noexcept
        {
            std::size_t index = 0;

            while (index < grid.children.size() && grid.children[index]->kind != ElementKind::SCROLL)
                ++index;
            return index;
        }
    }

    /*
    ** Порядок детей после этого шага: заголовок, [закреплённые сверху], прокрутка, [закреплённые снизу].
    ** Сохранённая раскладка (columnLayout) уже применена к столбцам (applyGridLayout) — таблица помнит, что она
    ** настроена, и при сохранении снова запишет раскладку в разметку.
    **
    ** The children order after this step: the header, [pinned at the top], the scroll, [pinned at the bottom].
    ** The saved layout (columnLayout) is already applied to the columns (applyGridLayout) — the table remembers it is
    ** customized and on saving writes the layout into the markup again.
    */
    void finishDataGrid(Node& grid, const GridOptions& options)
    {
        GridView& view = grid.content.grid->view;
        Node* rows = gridRows(grid);

        view.frozen = options.frozen;
        view.reorderable = options.reorderable;
        view.resizable = options.resizable;
        view.customized = !options.layout.empty();
        grid.content.grid->editable = options.editable;
        grid.content.grid->undoStack = options.undoStack;
        if (rows == nullptr || !rows->itemTemplate)
            return;
        // Сверху — прямо над прокруткой: под заголовками и строкой фильтров; место ищется, а не берётся «последним»
        // (ревью Т-72) / At the top — right above the scroll: under the headers and the filter row; the place is searched
        // for rather than taken as "the last" (review T-72)
        if (!options.pinnedTop.empty())
            insertChild(grid, scrollIndex(grid), makePinned(grid, *rows->itemTemplate, options.pinnedTop));
        if (!options.pinnedBottom.empty())
            appendChild(grid, makePinned(grid, *rows->itemTemplate, options.pinnedBottom));
    }
}
