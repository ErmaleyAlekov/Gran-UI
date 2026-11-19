#include "gran/ui/tree_view.hpp"
#include "gran/style/property.hpp"

#include <cmath>
#include <utility>

namespace gran::ui
{
    namespace
    {
        // Узел, который строит движок: место в разметке — у владельца / A node the engine builds: the markup place is the owner's
        std::unique_ptr<Node> makePart(const Node& owner, ElementKind kind)
        {
            std::unique_ptr<Node> node = std::make_unique<Node>();

            node->kind = kind;
            node->line = owner.line;
            node->column = owner.column;
            return node;
        }

        // Отступ числом без хвоста «.000000» (выражению XPath годится и дробный) / The indent as a number without a ".000000" tail (a fractional one suits XPath too)
        std::string indentText(float indent)
        {
            if (indent == std::floor(indent))
                return std::to_string(static_cast<long long>(indent));
            return std::to_string(indent);
        }

        // Подпись строки по умолчанию: поле записи, длинное — с многоточием / The default row label: the record field, a long one with an ellipsis
        std::unique_ptr<Node> makeLabel(const Node& view, const std::string& field)
        {
            std::unique_ptr<Node> label = makePart(view, ElementKind::TEXT);

            if (!field.empty())
                label->content.text = "{" + field + "}";
            style::setDeclaration(label->declared.base, style::PropertyId::GROW, 1.0f);
            style::setDeclaration(label->declared.base, style::PropertyId::TEXT_OVERFLOW,
                static_cast<int>(style::TextOverflow::ELLIPSIS));
            return label;
        }

        // Шаблон строки: кнопка раскрытия, затем содержимое (своё или подпись поля) / The row template: the expander button, then the content (own or the field label)
        std::unique_ptr<Node> makeRowTemplate(Node& view, const TreeViewOptions& options)
        {
            std::unique_ptr<Node> row = makePart(view, ElementKind::TREE_ROW);

            appendChild(*row, makeTreeExpander(view, options.indent));
            if (view.itemTemplate)
                appendChild(*row, std::move(view.itemTemplate));
            else
                appendChild(*row, makeLabel(view, options.field));
            return row;
        }
    }

    /*
    ** Сдвиг — левое поле кнопки, а не отдельный узел: правка ячейки столбца-дерева прячет содержимое ячейки,
    ** но оставляет кнопку, и редактор стоит на месте текста. Число в выражении — без знаков, которые XPath не поймёт.
    **
    ** The shift is the left margin of the button, not a separate node: editing a tree column cell hides the cell content
    ** but keeps the button, and the editor stands in place of the text. The number in the expression has no signs XPath does not understand.
    */
    std::unique_ptr<Node> makeTreeExpander(const Node& owner, float indent)
    {
        std::unique_ptr<Node> expander = makePart(owner, ElementKind::TREE_EXPANDER);

        expander->content.onClick = "gran:toggle";
        expander->content.bindings.push_back(
            AttributeBinding{"margin", "{concat('0 0 0 ', $depth * " + indentText(indent) + ")}"});
        expander->content.bindings.push_back(AttributeBinding{"checked", "{$expanded}"});
        expander->content.bindings.push_back(AttributeBinding{"value", "{$hasChildren}"});
        style::setDeclaration(expander->declared.base, style::PropertyId::SHRINK, 0.0f);
        return expander;
    }

    /*
    ** Порядок детей дерева: [0] прокрутка со списком строк — на него опирается treeViewRows. Прокрутка растягивается
    ** на всю высоту дерева (grow) и идёт только по вертикали: строка во всю ширину (подсветка выделения — тоже),
    ** длинная подпись обрезается многоточием, как в проводнике с узкой панелью.
    **
    ** The tree children order: [0] the scroll with the row list — treeViewRows relies on it. The scroll stretches
    ** over the whole tree height (grow) and goes only vertically: a row is full width (the selection highlight too),
    ** a long label is cut by an ellipsis, as in a file manager with a narrow pane.
    */
    void buildTreeView(Node& view, const TreeViewOptions& options)
    {
        std::unique_ptr<Node> scroll = makePart(view, ElementKind::SCROLL);
        std::unique_ptr<Node> rows = makePart(view, ElementKind::STACK);

        rows->content.items = std::move(view.content.items);
        rows->content.selection = std::move(view.content.selection);
        rows->content.virtualize = view.content.virtualize;
        rows->itemTemplate = makeRowTemplate(view, options);
        view.content.items.clear();
        view.content.selection.clear();
        view.children.clear();
        style::setDeclaration(scroll->declared.base, style::PropertyId::GROW, 1.0f);
        scroll->content.direction = ScrollDirection::VERTICAL;
        appendChild(*scroll, std::move(rows));
        appendChild(view, std::move(scroll));
    }
}
