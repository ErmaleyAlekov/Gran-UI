#include "gran/markup/internal/build_context.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Логический атрибут или значение по умолчанию (значение проверила схема) / A boolean attribute or the default (the schema has checked the value)
        bool flagOf(const beresta::Element& element, const char* name, bool fallback)
        {
            std::optional<std::string> value = element.getAttribute(name);

            if (!value)
                return fallback;
            return style::parseBoolean(*value);
        }

        // Строковый атрибут или пусто / A string attribute or empty
        std::string textOf(const beresta::Element& element, const char* name)
        {
            return element.getAttribute(name).value_or(std::string());
        }
    }

    /*
    ** Закреплённых столбцов — число 0…1000 (схема); больше столбцов, чем есть, закреплять нечего — предел
    ** накладывает строка (ui::gridFrozen). По умолчанию пользователь может и переставлять, и тянуть столбцы.
    **
    ** The frozen columns are a number 0…1000 (the schema); more than the existing columns cannot be frozen — the line
    ** applies the limit (ui::gridFrozen). By default the user can both reorder and resize the columns.
    ** Правка ячеек — только по явному editable="true": таблица только для просмотра не должна править данные случайно.
    ** Cell editing — only by an explicit editable="true": a view-only table must not edit the data by accident.
    */
    ui::GridOptions readGridOptions(const beresta::Element& element)
    {
        ui::GridOptions options;
        std::optional<std::string> frozen = element.getAttribute("frozenColumns");

        if (frozen)
            options.frozen = static_cast<std::size_t>(style::parseInteger(*frozen, 0, 1000));
        options.reorderable = flagOf(element, "reorderable", true);
        options.resizable = flagOf(element, "resizable", true);
        options.layout = textOf(element, "columnLayout");
        options.pinnedTop = textOf(element, "pinnedTop");
        options.pinnedBottom = textOf(element, "pinnedBottom");
        options.editable = flagOf(element, "editable", false);
        options.undoStack = textOf(element, "undoStack");
        options.filterRow = flagOf(element, "filterRow", false);
        return options;
    }

    // Только у DataGrid: у прочих элементов эти имена не значат ничего / Only for DataGrid: for other elements these names mean nothing
    bool isGridAttribute(const ui::Node& node, std::string_view name) noexcept
    {
        if (node.kind != ui::ElementKind::DATA_GRID)
            return false;
        return name == "frozenColumns" || name == "reorderable" || name == "resizable" || name == "columnLayout"
            || name == "pinnedTop" || name == "pinnedBottom" || name == "editable" || name == "undoStack" || name == "filterRow";
    }
}
