#include "gran/markup/internal/components.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

#include <string>

namespace gran::markup::internal
{
    namespace
    {
        /*
        ** Общие атрибуты, содержимое и свойства стиля. Действие «{$имя}» схема пропускает ради параметров
        ** компонентов; дошло до построения — значит, его не подставила развёртка (вне тела или нет такого Param).
        **
        ** The common attributes, the content and the style properties. The schema lets an action "{$name}" pass for the component
        ** parameters; reaching the build means no expansion substituted it (outside a body or no such Param).
        */
        void readNodeAttributes(const beresta::Element& element, ui::Node& node, BuildContext& context)
        {
            for (const beresta::Attribute& attribute : element.getAttributes())
            {
                if (attribute.name.rfind("on", 0) == 0 && attribute.value.rfind("{", 0) == 0)
                    throwAttributeError(element, attribute.name, i18n::format(i18n::Message::MARKUP_PARAM_OUTSIDE));
                if (attribute.name == "id")
                    node.id = attribute.value;
                else if (attribute.name == "style")
                    node.rule = context.document.styles.find(attribute.value);
                else if (readBindingAttribute(element, attribute, node))
                    continue;
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

        /*
        ** Пункты ComboBox уходят во внутренний слой Popup вместе с моделью списка (items, шаблон):
        ** так весь код меню (размещение, стрелки, выбор, привязки) работает и для списка. Слой
        ** берёт стиль темы Popup; подпись выбранного пункта ставится сразу (пункты из данных — после привязки).
        **
        ** The ComboBox items go into an inner Popup layer together with the list model (items, the template):
        ** this way all the menu code (placing, arrows, choosing, bindings) works for the list too. The layer
        ** takes the theme Popup style; the selected item label is set at once (the items from data — after binding).
        */
        void wrapOptions(ui::Node& combo)
        {
            std::unique_ptr<ui::Node> list = std::make_unique<ui::Node>();

            list->kind = ui::ElementKind::POPUP;
            list->visible = false;
            list->line = combo.line;
            list->column = combo.column;
            list->content.items = std::move(combo.content.items);
            list->itemTemplate = std::move(combo.itemTemplate);
            combo.content.items.clear();
            for (std::unique_ptr<ui::Node>& child : combo.children)
                ui::appendChild(*list, std::move(child));
            combo.children.clear();
            ui::appendChild(combo, std::move(list));
            ui::syncComboBoxes(combo);
        }

        // Пункт меню или строки меню (не ComboBox): только у них «_» в подписи — буква доступа
        // An item of a menu or a menu bar (not a ComboBox): only for them "_" in the label is the access letter
        bool inMenu(const beresta::Element& element) noexcept
        {
            const beresta::ContainerNode* parent = element.getParent();

            if (parent == nullptr || parent->getType() != beresta::NodeType::ELEMENT)
                return false;
            std::string_view name = static_cast<const beresta::Element*>(parent)->getLocalName();
            return name == "Popup" || name == "MenuBar";
        }
    }

    /*
    ** Узел из элемента: атрибуты, состояния, текст тега (у Text, Button и TextArea), затем дети
    ** и шаблон списка, выражения {…} в текстах; у ComboBox — слой списка, у Tabs — полоса заголовков.
    ** Глубину рекурсии ограничивает парсер (maxDepth = 256).
    **
    ** A node from an element: the attributes, the states, the tag text (for Text, Button and TextArea), then the children
    ** and the list template, the {…} expressions in the texts; for a ComboBox — the list layer, for Tabs — the header strip.
    ** The parser bounds the recursion depth (maxDepth = 256).
    ** Экземпляр компонента разворачивается в тело (component_expand.cpp).
    ** A component instance expands into the body (component_expand.cpp).
    */
    std::unique_ptr<ui::Node> buildNode(const beresta::Element& element, BuildContext& context)
    {
        if (isComponentInstance(element))
            return expandComponent(element, context);
        std::unique_ptr<ui::Node> node = std::make_unique<ui::Node>();

        countNode(element, context);
        node->domElement = &element;
        elementKind(element.getLocalName(), node->kind);
        node->line = element.getSourceLocation().line;
        node->column = element.getSourceLocation().column;
        readNodeAttributes(element, *node, context);
        readStates(element, node->declared, context);
        if (node->kind == ui::ElementKind::ICON)
            readIconShape(element, *node);
        if (!readTagText(element, *node, context))
            buildChildren(element, *node, context);
        readTextBindings(element, *node);
        if (node->kind == ui::ElementKind::MENU_ITEM && inMenu(element))
            ui::takeMnemonic(*node);
        if (node->kind == ui::ElementKind::COMBO_BOX)
            wrapOptions(*node);
        if (node->kind == ui::ElementKind::TABS)
            ui::syncTabs(*node);
        if (node->kind == ui::ElementKind::TOOL_BAR)
            ui::buildToolBarMore(*node);
        if (node->kind == ui::ElementKind::STATUS_BAR && node->content.sizeGrip)
            ui::buildSizeGrip(*node);
        return node;
    }
}
