#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <string>

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

    // Позиция — в тексте (как у ошибок атрибутов) и в полях исключения / The position goes into the text (as for attribute errors) and into the exception fields
    void throwElementError(const beresta::Element& element, const std::string& message)
    {
        const beresta::SourceLocation& location = element.getSourceLocation();

        throw MarkupException(ErrorCode::MARKUP, i18n::format(i18n::Message::MARKUP_AT_POSITION,
            {std::to_string(location.line), std::to_string(location.column), message}), location.line, location.column);
    }
}
