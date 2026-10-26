#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

#include <string>

namespace gran::markup::internal
{
    namespace
    {
        // Вид элемента по имени тега; false — это не элемент интерфейса (а состояние)
        // The element kind by the tag name; false — it is not a UI element (but a state)
        bool elementKind(std::string_view name, ui::ElementKind& kind) noexcept
        {
            for (int index = 0; index <= static_cast<int>(ui::ElementKind::MENU_ITEM); ++index)
            {
                if (name == ui::elementName(static_cast<ui::ElementKind>(index)))
                {
                    kind = static_cast<ui::ElementKind>(index);
                    return true;
                }
            }
            return false;
        }

        // Общие атрибуты, содержимое и свойства стиля / The common attributes, the content and the style properties
        void readNodeAttributes(const beresta::Element& element, ui::Node& node, BuildContext& context)
        {
            for (const beresta::Attribute& attribute : element.getAttributes())
            {
                if (attribute.name == "id")
                    node.id = attribute.value;
                else if (attribute.name == "style")
                    node.rule = context.document.styles.find(attribute.value);
                else if (attribute.name == "visible")
                    node.visible = style::parseBoolean(attribute.value);
                else if (attribute.name == "enabled")
                    node.enabled = style::parseBoolean(attribute.value);
                else if (!readContentAttribute(node, attribute.name, attribute.value))
                    readDeclaration(element, attribute, node.declared.base, context);
            }
        }

        // Предел числа узлов защищает от разметки-бомбы / The node count limit protects against a markup bomb
        void countNode(const beresta::Element& element, BuildContext& context)
        {
            if (++context.nodeCount <= context.options.maxNodes)
                return;
            const beresta::SourceLocation& location = element.getSourceLocation();
            throw MarkupException(ErrorCode::LIMIT_EXCEEDED, i18n::format(i18n::Message::MARKUP_TOO_MANY_NODES,
                {std::to_string(context.options.maxNodes)}), location.line, location.column);
        }
    }

    /*
    ** Узел из элемента: атрибуты, состояния, текст тега (у Text, Button и TextArea), затем дети.
    ** Глубину рекурсии ограничивает парсер (maxDepth = 256).
    **
    ** A node from an element: the attributes, the states, the tag text (for Text, Button and TextArea), then the children.
    ** The parser bounds the recursion depth (maxDepth = 256).
    */
    std::unique_ptr<ui::Node> buildNode(const beresta::Element& element, BuildContext& context)
    {
        std::unique_ptr<ui::Node> node = std::make_unique<ui::Node>();
        ui::ElementKind childKind = ui::ElementKind::BOX;

        countNode(element, context);
        elementKind(element.getLocalName(), node->kind);
        node->line = element.getSourceLocation().line;
        node->column = element.getSourceLocation().column;
        readNodeAttributes(element, *node, context);
        readStates(element, node->declared, context);
        if (readTagText(element, *node, context))
            return node;
        for (const beresta::Element* child = firstElementChild(element); child != nullptr;
             child = nextElementSibling(*child))
        {
            if (elementKind(child->getLocalName(), childKind))
                ui::appendChild(*node, buildNode(*child, context));
        }
        return node;
    }
}
