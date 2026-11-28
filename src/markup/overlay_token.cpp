#include "gran/markup/internal/overlay.hpp"
#include "gran/markup/internal/components.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Заменяем целый токен только в свойствах стиля, пропуская выражения привязок.
        // Replace whole tokens only in style properties and skip binding expressions.
        void replaceReferences(beresta::Element& element, const std::string& name, const std::string& value)
        {
            for (const auto& attribute : element.getAttributes())
            {
                if (!style::findProperty(attribute.name) || attribute.value.find('{') != std::string::npos)
                    continue;
                std::string text = attribute.value;
                std::size_t position = 0;
                while ((position = text.find('$', position)) != std::string::npos)
                {
                    const auto end = text.find_first_not_of(
                        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-", position + 1);
                    if (text.substr(position + 1, end - position - 1) == name)
                    {
                        text.replace(position, name.size() + 1, value);
                        position += value.size();
                    }
                    else if (end == std::string::npos)
                        break;
                    else
                        position = end;
                }
                if (text != attribute.value)
                    element.setAttribute(attribute.name, text);
            }
        }

    }

    // Применяем значения области только к свойствам этого элемента.
    // Apply regional values only to the properties of this element.
    void replaceScopedProperties(beresta::Element& element, const TokenMap& tokens)
    {
        for (const auto& [name, value] : tokens)
            replaceReferences(element, name, value);
    }

    // Значение области проверяем по исходному объявлению и его границам.
    // Validate a regional value against the original declaration and its bounds.
    void applyToken(OverlayContext& context, const beresta::Element& op)
    {
        auto* scope = op.hasAttribute("id") ? overlayTarget(context, op, "id") : nullptr;
        if (!scope && op.hasAttribute("id"))
            return;
        if (scope)
            requirePermission(*scope, Customization::STYLE);
        const auto& name = *findAttribute(op, "name");
        auto* definition = overlayResource(context, "Theme", "Token", "name", name);
        if (!definition || !definition->hasAttribute("type"))
            denyOverlay(op, name);
        auto& token = const_cast<beresta::Element&>(*definition);
        const auto previous = *findAttribute(token, "value");
        const auto& value = *findAttribute(op, "value");
        token.setAttribute("value", value);
        validateEditableToken(token);
        checkSchema(*context.candidate.dom, documentSchema(context.candidate));
        if (scope)
        {
            token.setAttribute("value", previous);
            context.scoped[scope][name] = value;
        }
    }
}
