#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Ближайший элемент начиная с узла node / The nearest element starting from the node
        const beresta::Element* elementFrom(const beresta::Node* node) noexcept
        {
            while (node != nullptr && node->getType() != beresta::NodeType::ELEMENT)
                node = node->getNextSibling();
            return static_cast<const beresta::Element*>(node);
        }
    }

    // Текст и комментарии между элементами пропускаются / Text and comments between elements are skipped
    const beresta::Element* firstElementChild(const beresta::Element& element) noexcept
    {
        return elementFrom(element.getFirstChild());
    }

    // Следующий элемент того же родителя / The next element of the same parent
    const beresta::Element* nextElementSibling(const beresta::Element& element) noexcept
    {
        return elementFrom(element.getNextSibling());
    }

    // Атрибуты в разметке UI немногочисленны: линейный поиск / UI markup attributes are few: a linear search
    const std::string* findAttribute(const beresta::Element& element, std::string_view name) noexcept
    {
        for (const beresta::Attribute& attribute : element.getAttributes())
        {
            if (attribute.name == name)
                return &attribute.value;
        }
        return nullptr;
    }
}
