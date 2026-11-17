#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/ui/data_grid.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Способ сравнения по имени (перечисление проверила схема) / The comparison by its name (the schema has checked the enumeration)
        ui::GridSortType sortTypeOf(std::string_view name) noexcept
        {
            if (name == "text")
                return ui::GridSortType::TEXT;
            if (name == "number")
                return ui::GridSortType::NUMBER;
            if (name == "natural")
                return ui::GridSortType::NATURAL;
            return ui::GridSortType::AUTO;
        }

        // Размер столбца: ширина — одна дорожка сетки, пределы — числа; подстановка темы ($токен) — как у свойств
        // A column size: the width is one grid track, the limits are numbers; the theme substitution ($token) — as for the properties
        void readColumnSize(const beresta::Element& element, const beresta::Attribute& attribute, ui::GridColumn& column,
            BuildContext& context)
        {
            try
            {
                std::string text = substituteTokens(attribute.value, context);
                if (attribute.name == "minWidth")
                    column.minWidth = style::parseNumber(text);
                else if (attribute.name == "maxWidth")
                    column.maxWidth = style::parseNumber(text);
                else
                    column.width = style::parseTrack(text);
            }
            catch (const GranException& error)
            {
                throwAttributeError(element, attribute.name, error.getMessage());
            }
        }

        // Атрибуты столбца; прочие атрибуты — свойства стиля текста ячеек поля / The column attributes; the other attributes are the style properties of the field cell text
        void readColumnAttribute(const beresta::Element& element, const beresta::Attribute& attribute,
            ui::GridColumnSpec& spec, BuildContext& context)
        {
            if (attribute.name == "field")
                spec.column.field = attribute.value;
            else if (attribute.name == "title")
                spec.column.title = attribute.value;
            else if (attribute.name == "sortable")
                spec.column.sortable = style::parseBoolean(attribute.value);
            else if (attribute.name == "sortType")
                spec.column.sortType = sortTypeOf(attribute.value);
            else if (attribute.name == "width" || attribute.name == "minWidth" || attribute.name == "maxWidth")
                readColumnSize(element, attribute, spec.column, context);
            else
                readDeclaration(element, attribute, spec.cellStyle, context);
        }

        /*
        ** <GridColumn>: ширина по умолчанию — 1fr (таблица занимает всю ширину), шаблон <CellTemplate> —
        ** ровно один элемент интерфейса (проверила схема); он строится как обычный узел со своими привязками.
        **
        ** <GridColumn>: the default width is 1fr (the table takes the whole width), the <CellTemplate> template is
        ** exactly one UI element (the schema has checked it); it is built as an ordinary node with its own bindings.
        */
        ui::GridColumnSpec readColumn(const beresta::Element& element, BuildContext& context)
        {
            ui::GridColumnSpec spec;

            spec.column.width = style::parseTrack("1fr");
            for (const beresta::Attribute& attribute : element.getAttributes())
                readColumnAttribute(element, attribute, spec, context);
            const beresta::Element* cell = firstElementChild(element);
            if (cell != nullptr && cell->getLocalName() == "CellTemplate" && firstElementChild(*cell) != nullptr)
                spec.cellTemplate = buildNode(*firstElementChild(*cell), context);
            return spec;
        }
    }

    // Таблица без virtual — виртуальная: таблицы обычно большие, а копии строк вне экрана не нужны / A table without virtual is virtual: tables are usually big, and row copies off the screen are not needed
    void readDataGrid(const beresta::Element& element, ui::Node& node, BuildContext& context)
    {
        std::vector<ui::GridColumnSpec> specs;

        for (const beresta::Element* child = firstElementChild(element); child != nullptr;
             child = nextElementSibling(*child))
        {
            if (child->getLocalName() == "GridColumn")
                specs.push_back(readColumn(*child, context));
        }
        if (!element.hasAttribute("virtual"))
            node.content.virtualize = true;
        ui::buildDataGrid(node, specs);
    }
}
