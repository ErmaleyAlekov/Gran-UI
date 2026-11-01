#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Собственный текст элемента (прямые дети TEXT и CDATA); только пробелы — пусто: это отступы, не содержимое
        // The element own text (the direct TEXT and CDATA children); whitespace only — empty: it is indentation, not content
        std::string ownText(const beresta::Element& element)
        {
            std::string text;

            for (const beresta::Node* child = element.getFirstChild(); child != nullptr; child = child->getNextSibling())
            {
                beresta::NodeType type = child->getType();
                if (type == beresta::NodeType::TEXT || type == beresta::NodeType::CDATA_SECTION)
                    text += static_cast<const beresta::CharacterNode*>(child)->getValue();
            }
            if (text.find_first_not_of(" \t\r\n") == std::string::npos)
                return std::string();
            return text;
        }

        // Дети-элементы, которые не элементы интерфейса: состояния, <ItemTemplate>, участки <Span> не сюда (они — интерфейс)
        // The element children that are not UI elements: the states, <ItemTemplate>; the <Span> runs are not here (they are UI)
        std::vector<const beresta::Element*> otherChildren(const beresta::Element& element)
        {
            std::vector<const beresta::Element*> result;

            for (const beresta::Element* child = firstElementChild(element); child != nullptr; child = nextElementSibling(*child))
            {
                if (!isUiElement(*child))
                    result.push_back(child);
            }
            return result;
        }
    }

    // Те же правила, что у buildChildren: элемент интерфейса — имя из списка видов или экземпляр компонента / The same rules as in buildChildren: a UI element is a name from the kind list or a component instance
    std::vector<const beresta::Element*> uiElementChildren(const beresta::Element& element)
    {
        std::vector<const beresta::Element*> result;

        for (const beresta::Element* child = firstElementChild(element); child != nullptr; child = nextElementSibling(*child))
        {
            if (isUiElement(*child))
                result.push_back(child);
        }
        return result;
    }

    // Порядок атрибутов тоже в счёт: перестановка — редкость, перестроить узел дешевле, чем сравнивать множества
    // The attribute order counts too: a reordering is rare, rebuilding the node is cheaper than comparing sets
    bool sameAttributes(const beresta::Element& left, const beresta::Element& right) noexcept
    {
        const std::vector<beresta::Attribute>& first = left.getAttributes();
        const std::vector<beresta::Attribute>& second = right.getAttributes();

        if (first.size() != second.size())
            return false;
        for (std::size_t index = 0; index < first.size(); ++index)
        {
            if (first[index].name != second[index].name || first[index].value != second[index].value)
                return false;
        }
        return true;
    }

    // Состояния и шаблон сравниваются целиком: они влияют на сам узел, а не на его детей
    // The states and the template are compared as a whole: they affect the node itself, not its children
    bool sameShell(const beresta::Element& left, const beresta::Element& right)
    {
        if (left.getName() != right.getName() || !sameAttributes(left, right) || ownText(left) != ownText(right))
            return false;
        std::vector<const beresta::Element*> first = otherChildren(left);
        std::vector<const beresta::Element*> second = otherChildren(right);
        if (first.size() != second.size())
            return false;
        for (std::size_t index = 0; index < first.size(); ++index)
        {
            if (!first[index]->isEqualNode(*second[index]))
                return false;
        }
        return true;
    }
}
