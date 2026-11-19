#include "gran/markup/internal/build_context.hpp"
#include "gran/ui/data_grid.hpp"

namespace gran::markup::internal
{
    // Способ сравнения по имени (перечисление проверила схема) / The comparison by its name (the schema has checked the enumeration)
    ui::GridSortType gridSortType(std::string_view name) noexcept
    {
        if (name == "text")
            return ui::GridSortType::TEXT;
        if (name == "number")
            return ui::GridSortType::NUMBER;
        if (name == "natural")
            return ui::GridSortType::NATURAL;
        return ui::GridSortType::AUTO;
    }

    // Тип значений для правки по имени (перечисление проверила схема) / The value type for editing by its name (the schema has checked the enumeration)
    ui::GridEditType gridEditType(std::string_view name) noexcept
    {
        if (name == "text")
            return ui::GridEditType::TEXT;
        if (name == "number")
            return ui::GridEditType::NUMBER;
        if (name == "date")
            return ui::GridEditType::DATE;
        if (name == "boolean")
            return ui::GridEditType::BOOLEAN;
        return ui::GridEditType::AUTO;
    }
}
