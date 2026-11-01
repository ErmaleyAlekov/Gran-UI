#include "gran/markup/internal/components.hpp"
#include "gran/core/gran_exception.hpp"

#include "beresta.hpp"

#include <utility>
#include <vector>

namespace gran::markup::internal
{
    namespace
    {
        /*
        ** Копия тела отрывается от <Library>, а с ним и от объявлений xmlns: префиксы вложенных
        ** экземпляров (<c:Badge>) перестали бы разрешаться. Ближайшее объявление выигрывает.
        **
        ** The body copy is torn off <Library>, and with it off the xmlns declarations: the prefixes of the nested
        ** instances (<c:Badge>) would stop resolving. The nearest declaration wins.
        */
        void copyNamespaces(const beresta::Element& source, beresta::Element& root)
        {
            for (const beresta::ContainerNode* node = source.getParent(); node != nullptr; node = node->getParent())
            {
                if (node->getType() != beresta::NodeType::ELEMENT)
                    continue;
                for (const beresta::Attribute& attribute : static_cast<const beresta::Element*>(node)->getAttributes())
                {
                    if (attribute.name.rfind("xmlns", 0) == 0 && !root.hasAttribute(attribute.name))
                        root.setAttribute(attribute.name, attribute.value);
                }
            }
        }

        /*
        ** id, labeledBy, popup — локальные имена тела; группа переключателей тоже своя у экземпляра; style ищется
        ** в компоненте → библиотеке → окне; прочее — подстановка параметров и id в выражениях {…}.
        ** nullopt — атрибут пропадает (целиком «{$имя}» у незаданного параметра).
        **
        ** id, labeledBy, popup are the body local names; the radio group is the instance own too; style is looked up
        ** in the component → the library → the window; the rest is the parameter and id substitution in the {…} expressions.
        ** nullopt — the attribute disappears (a whole "{$name}" of an unset parameter).
        */
        std::optional<std::string> rewriteValue(const beresta::Element& element, const beresta::Attribute& attribute,
            const Substitution& substitution)
        {
            const std::string& name = attribute.name;

            if (name == "id" || name == "labeledBy" || name == "popup")
            {
                auto found = substitution.ids.find(attribute.value);
                if (found != substitution.ids.end())
                    return found->second;
                return attribute.value;
            }
            if (name == "group" && element.getLocalName() == "RadioButton")
                return substitution.prefix + "." + attribute.value;
            if (name.rfind("xmlns", 0) == 0)
                return attribute.value;
            if (name != "style")
                return substituteAttribute(attribute.value, substitution);
            try
            {
                return resolveStyleName(attribute.value, substitution.spec->styles, *substitution.styles);
            }
            catch (const GranException& error)
            {
                throwAttributeError(element, name, error.getMessage());
            }
        }

        // Копия списка: снятие и замена атрибутов меняют сам список элемента / A copy of the list: removing and replacing attributes change the element list itself
        void rewriteAttributes(beresta::Element& element, const Substitution& substitution)
        {
            std::vector<beresta::Attribute> attributes = element.getAttributes();

            for (const beresta::Attribute& attribute : attributes)
            {
                std::optional<std::string> value = rewriteValue(element, attribute, substitution);
                if (!value.has_value())
                    element.removeAttribute(attribute.name);
                else if (*value != attribute.value)
                    element.setAttribute(attribute.name, std::move(*value));
            }
        }

        /*
        ** Атрибуты и тексты всего поддерева; HTML у <WebView> не трогается (в CSS и JavaScript скобки обычны).
        ** Вложенный экземпляр без id получает «приставка.Компонент-строка-столбец»: иначе его id совпали бы
        ** у всех экземпляров внешнего компонента.
        **
        ** The attributes and texts of the whole subtree; the <WebView> HTML is not touched (brackets are usual in CSS and JavaScript).
        ** A nested instance without an id gets "prefix.Component-line-column": otherwise its ids would match
        ** in all the instances of the outer component.
        */
        void rewriteTree(beresta::Element& element, const Substitution& substitution)
        {
            bool html = element.getLocalName() == "WebView";
            const beresta::SourceLocation& location = element.getSourceLocation();

            rewriteAttributes(element, substitution);
            if (isComponentInstance(element) && !element.hasAttribute("id"))
                element.setAttribute("id", substitution.prefix + "." + std::string(element.getLocalName()) + "-"
                    + std::to_string(location.line) + "-" + std::to_string(location.column));
            for (std::size_t index = 0; index < element.getChildCount(); ++index)
            {
                beresta::Node& child = *element.getChild(index);
                beresta::NodeType type = child.getType();
                if (type == beresta::NodeType::ELEMENT)
                    rewriteTree(static_cast<beresta::Element&>(child), substitution);
                else if (!html && (type == beresta::NodeType::TEXT || type == beresta::NodeType::CDATA_SECTION))
                {
                    beresta::CharacterNode& text = static_cast<beresta::CharacterNode&>(child);
                    text.setValue(substituteText(text.getValue(), substitution));
                }
            }
        }
    }

    /*
    ** Своя копия тела на каждый экземпляр (документ «Берёсты» без родителя), атрибуты хозяина
    ** (id, visible, компоновка) — поверх атрибутов корня: экземпляр решает, где и каким размером стоит.
    **
    ** An own body copy per instance (a Beresta document without a parent), the host attributes
    ** (id, visible, the layout) go over the root attributes: the instance decides where and how big it stands.
    */
    std::shared_ptr<beresta::Document> makeShadow(const beresta::Element& instance, const Substitution& substitution)
    {
        std::shared_ptr<beresta::Document> shadow = std::make_shared<beresta::Document>();
        std::unique_ptr<beresta::Node> copy = substitution.spec->body->clone();
        beresta::Element& root = *shadow->setRoot(std::unique_ptr<beresta::Element>(static_cast<beresta::Element*>(copy.release())));

        copyNamespaces(*substitution.spec->body, root);
        rewriteTree(root, substitution);
        for (const beresta::Attribute& attribute : instance.getAttributes())
        {
            if (isHostAttribute(attribute.name))
                root.setAttribute(attribute.name, attribute.value);
        }
        return shadow;
    }
}
