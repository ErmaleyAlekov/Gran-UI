#include "gran/markup/internal/build_context.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/ui/data_grid.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    /*
    ** Атрибуты дерева у столбца таблицы: tree (столбец-дерево), indent (сдвиг уровня), filterable (поле в строке
    ** фильтров). Значения проверила схема: логические — xs:boolean, отступ — неотрицательное число.
    **
    ** The tree attributes of a table column: tree (a tree column), indent (the level shift), filterable (an input in the filter
    ** row). The schema has checked the values: the booleans are xs:boolean, the indent is a non-negative number.
    */
    bool readTreeColumnAttribute(std::string_view name, const std::string& value, ui::GridColumn& column)
    {
        if (name == "tree")
            column.tree = style::parseBoolean(value);
        else if (name == "indent")
            column.indent = style::parseNumber(value);
        else if (name == "filterable")
            column.filterable = style::parseBoolean(value);
        else
            return false;
        return true;
    }

    // Настройки <TreeView>: поле подписи и отступ уровня (по умолчанию ui::TREE_INDENT) / The <TreeView> settings: the label field and the level indent (ui::TREE_INDENT by default)
    ui::TreeViewOptions readTreeViewOptions(const beresta::Element& element)
    {
        ui::TreeViewOptions options;
        std::optional<std::string> indent = element.getAttribute("indent");

        options.field = element.getAttribute("field").value_or(std::string());
        if (indent)
            options.indent = style::parseNumber(*indent);
        return options;
    }

    // Только у TreeView: у прочих элементов эти имена не значат ничего / Only for TreeView: for other elements these names mean nothing
    bool isTreeViewAttribute(const ui::Node& node, std::string_view name) noexcept
    {
        return node.kind == ui::ElementKind::TREE_VIEW && (name == "field" || name == "indent");
    }

    /*
    ** <TreeView>: дети уже построены (единственный — <ItemTemplate>, его проверила схема и он лежит в itemTemplate);
    ** строки без virtual — виртуальные, как у таблицы: деревья бывают большими (папки, справочники).
    **
    ** <TreeView>: the children are already built (the only one is <ItemTemplate>, the schema has checked it and it lies in itemTemplate);
    ** the rows without virtual are virtual, as for a table: trees can be big (folders, reference books).
    */
    void readTreeView(const beresta::Element& element, ui::Node& node)
    {
        if (!element.hasAttribute("virtual"))
            node.content.virtualize = true;
        ui::buildTreeView(node, readTreeViewOptions(element));
    }
}
