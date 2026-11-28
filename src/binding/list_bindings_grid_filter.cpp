#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    namespace
    {
        // Условия всех столбцов с полем; числовой столбец понимает «>100» и подобные / The conditions of all the columns with a field; a numeric column understands ">100" and the like
        std::vector<data::ColumnCondition> conditionsOf(const ui::GridState& state, const data::ListSource& source)
        {
            std::vector<data::ColumnCondition> conditions;

            for (const ui::GridColumn& column : state.columns)
            {
                if (column.field.empty() || column.filterText.empty())
                    continue;
                conditions.push_back(data::ColumnCondition{column.field, column.filterText,
                    gridSortTypeOf(column, source) == data::SortType::NUMBER});
            }
            return conditions;
        }
    }

    bool ListBindings::filterColumn(const ui::Node& input)
    {
        ui::Node* grid = ui::gridOf(input);
        ui::Node* line = nullptr;
        ListBinding* binding = nullptr;

        if (grid != nullptr && ui::gridRows(*grid) != nullptr)
        {
            line = ui::gridFilterRow(*grid);
            binding = bindingOf(*ui::gridRows(*grid));
        }
        if (line == nullptr || binding == nullptr)
            return false;
        ui::GridState& state = *grid->content.grid;
        std::size_t column = ui::cellColumn(*line, input);
        data::ListSource& model = binding->model();
        if (column >= state.columns.size() || !filtersColumns(model))
            return false;
        state.columns[column].filterText = input.content.text;
        applyColumnConditions(model, conditionsOf(state, model));
        return true;
    }

    /*
    ** Таблица со строкой фильтров только что привязана (документ построен заново, ветка вставлена) или ей перенесли
    ** состояние полей: условия модели приводятся к тексту полей строки — иначе прокси остался бы отфильтрован прошлым
    ** экземпляром таблицы, а поле показывало бы одно, строки — другое.
    **
    ** A table with a filter row has just been bound (the document is rebuilt, a branch is inserted) or its input state
    ** was carried over: the model conditions are brought to the text of the row inputs — otherwise the proxy would stay
    ** filtered by a former table instance, and the input would show one thing while the rows show another.
    */
    void syncGridFilter(const ui::Node& rows, data::ListSource& model)
    {
        ui::Node* grid = ui::gridOf(rows);

        if (grid == nullptr || ui::gridRows(*grid) != &rows || ui::gridFilterRow(*grid) == nullptr)
            return;
        ui::readGridFilterTexts(*grid);
        applyColumnConditions(model, conditionsOf(*grid->content.grid, model));
    }

    // Обход поддерева: у каждой таблицы со строкой фильтров — синхронизация её модели / A subtree walk: every table with a filter row syncs its model
    bool ListBindings::refreshGridFilters(ui::Node& subtree)
    {
        ui::Node* rows = ui::gridRows(subtree);
        ListBinding* binding = nullptr;

        if (rows != nullptr)
            binding = bindingOf(*rows);
        bool refreshed = binding != nullptr && ownsColumnFilter(*rows);
        if (refreshed)
            syncGridFilter(*rows, binding->model());
        for (const std::unique_ptr<ui::Node>& child : subtree.children)
            refreshed |= refreshGridFilters(*child);
        return refreshed;
    }

    bool ownsColumnFilter(const ui::Node& rows) noexcept
    {
        ui::Node* grid = ui::gridOf(rows);

        return grid != nullptr && ui::gridRows(*grid) == &rows && ui::gridFilterRow(*grid) != nullptr;
    }
}
