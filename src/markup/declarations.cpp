#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <string>
#include <utility>

namespace gran::markup::internal
{
    // Ошибка с позицией элемента и именем атрибута / An error with the element position and the attribute name
    void throwAttributeError(const beresta::Element& element, std::string_view attribute, const std::string& message)
    {
        const beresta::SourceLocation& location = element.getSourceLocation();
        std::string where = i18n::format(i18n::Message::MARKUP_ATTRIBUTE_VALUE,
            {element.getLocalName(), attribute, message});

        throw MarkupException(ErrorCode::MARKUP, i18n::format(i18n::Message::MARKUP_AT_POSITION,
            {std::to_string(location.line), std::to_string(location.column), where}),
            location.line, location.column);
    }

    /*
    ** Подстановка "$имя" значением темы. Имя — буквы, цифры, '_' и '-'. Значения темы
    ** сами не содержат '$' (утверждение схемы), поэтому подстановка однопроходная.
    **
    ** Substituting "$name" with a theme value. A name is letters, digits, '_' and '-'. Theme values
    ** contain no '$' themselves (a schema assertion), so the substitution is single-pass.
    */
    std::string substituteTokens(std::string_view value, const BuildContext& context)
    {
        std::string result;
        std::size_t position = 0;

        while (position < value.size())
        {
            std::size_t dollar = value.find('$', position);
            result.append(value.substr(position, dollar - position));
            if (dollar == std::string_view::npos)
                break;
            std::size_t end = value.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-", dollar + 1);
            std::string_view name = value.substr(dollar + 1, end - dollar - 1);
            auto token = context.tokens.find(name);
            if (token == context.tokens.end())
                throw GranException(ErrorCode::RESOURCE, i18n::format(i18n::Message::RESOURCE_UNKNOWN_TOKEN, {name}));
            result += token->second;
            position = end;
        }
        return result;
    }

    // Свойство стиля: подстановка темы, разбор, запись; ошибка — с позицией
    // A style property: theme substitution, parsing, storing; an error — with the position
    bool readDeclaration(const beresta::Element& element, const beresta::Attribute& attribute,
        style::PropertySet& set, BuildContext& context)
    {
        const style::PropertyInfo* info = style::findProperty(attribute.name);

        if (info == nullptr)
            return false;
        try
        {
            std::string text = substituteTokens(attribute.value, context);
            style::PropertyValue value = style::parsePropertyValue(*info, text);
            if (info->id == style::PropertyId::ANIMATION)
                checkAnimationNames(value, context);
            style::setDeclaration(set, info->id, std::move(value));
        }
        catch (const GranException& error)
        {
            throwAttributeError(element, attribute.name, error.getMessage());
        }
        return true;
    }

    // Все атрибуты-свойства; прочие (id, text, onClick...) обрабатывает построитель узла
    // All property attributes; the others (id, text, onClick...) are handled by the node builder
    void readDeclarations(const beresta::Element& element, style::PropertySet& set, BuildContext& context)
    {
        for (const beresta::Attribute& attribute : element.getAttributes())
            readDeclaration(element, attribute, set, context);
    }

    // Дочерние элементы состояний в порядке схемы / The state child elements in the schema order
    void readStates(const beresta::Element& element, style::DeclarationBlock& block, BuildContext& context)
    {
        static const char* const NAMES[style::STATE_COUNT] = {"Hover", "Pressed", "Focused", "Disabled", "Checked", "DropTarget"};

        for (const beresta::Element* child = firstElementChild(element); child != nullptr;
             child = nextElementSibling(*child))
        {
            for (std::size_t state = 0; state < style::STATE_COUNT; ++state)
            {
                if (child->getLocalName() == NAMES[state])
                    readDeclarations(*child, block.states[state], context);
            }
        }
    }
}
