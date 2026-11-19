#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/ui/data_grid.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
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
            else if (attribute.name == "id")
                spec.column.key = attribute.value;
            else if (attribute.name == "title")
                spec.column.title = attribute.value;
            else if (attribute.name == "sortable")
                spec.column.sortable = style::parseBoolean(attribute.value);
            else if (attribute.name == "resizable")
                spec.column.resizable = style::parseBoolean(attribute.value);
            else if (attribute.name == "sortType")
                spec.column.sortType = gridSortType(attribute.value);
            else if (attribute.name == "editable")
                spec.column.editable = style::parseBoolean(attribute.value);
            else if (attribute.name == "type")
                spec.column.editType = gridEditType(attribute.value);
            else if (attribute.name == "width" || attribute.name == "minWidth" || attribute.name == "maxWidth")
                readColumnSize(element, attribute, spec.column, context);
            else
                readDeclaration(element, attribute, spec.cellStyle, context);
        }

        /*
        ** <GridColumn>: ширина по умолчанию — 1fr (таблица занимает всю ширину), шаблоны <CellTemplate> (вид ячейки)
        ** и <EditTemplate> (редактор) — ровно один элемент интерфейса (проверила схема); они строятся как обычные
        ** узлы со своими привязками. Редактор только копируется при каждой правке — он общий у копий столбца.
        **
        ** <GridColumn>: the default width is 1fr (the table takes the whole width), the <CellTemplate> (the cell look)
        ** and <EditTemplate> (the editor) templates are exactly one UI element (the schema has checked it); they are built
        ** as ordinary nodes with their own bindings. The editor is only copied on every edit — it is shared by the column copies.
        */
        ui::GridColumnSpec readColumn(const beresta::Element& element, BuildContext& context)
        {
            ui::GridColumnSpec spec;

            spec.column.width = style::parseTrack("1fr");
            for (const beresta::Attribute& attribute : element.getAttributes())
                readColumnAttribute(element, attribute, spec, context);
            for (const beresta::Element* child = firstElementChild(element); child != nullptr;
                 child = nextElementSibling(*child))
            {
                const beresta::Element* root = firstElementChild(*child);
                if (root != nullptr && child->getLocalName() == "CellTemplate")
                    spec.cellTemplate = buildNode(*root, context);
                else if (root != nullptr && child->getLocalName() == "EditTemplate")
                    spec.column.editTemplate = buildNode(*root, context);
            }
            return spec;
        }
    }

    /*
    ** Таблица без virtual — виртуальная: таблицы обычно большие, а копии строк вне экрана не нужны. Ключ столбца
    ** в сохранённой раскладке — id, без него field, без обоих — «#номер в разметке». Раскладка (columnLayout)
    ** переставляет столбцы до построения: части таблицы сразу строятся в нужном порядке.
    **
    ** A table without virtual is virtual: tables are usually big, and row copies off the screen are not needed. The column key
    ** in the saved layout is id, without it field, without both — "#index in the markup". The layout (columnLayout)
    ** reorders the columns before building: the table parts are built in the right order at once.
    */
    void readDataGrid(const beresta::Element& element, ui::Node& node, BuildContext& context)
    {
        std::vector<ui::GridColumnSpec> specs;
        ui::GridOptions options = readGridOptions(element);

        for (const beresta::Element* child = firstElementChild(element); child != nullptr;
             child = nextElementSibling(*child))
        {
            if (child->getLocalName() != "GridColumn")
                continue;
            ui::GridColumnSpec spec = readColumn(*child, context);
            if (spec.column.key.empty())
                spec.column.key = spec.column.field;
            if (spec.column.key.empty())
                spec.column.key = "#" + std::to_string(specs.size());
            specs.push_back(std::move(spec));
        }
        if (!element.hasAttribute("virtual"))
            node.content.virtualize = true;
        ui::applyGridLayout(specs, options.layout);
        ui::buildDataGrid(node, specs, options);
    }
}
