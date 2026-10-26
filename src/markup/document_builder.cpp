#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Числовые параметры окна по имени атрибута / The numeric window parameters by attribute name
        struct WindowNumber
        {
            const char* name;
            float WindowSpec::* member;
        };

        const WindowNumber WINDOW_NUMBERS[] = {
            {"width", &WindowSpec::width}, {"height", &WindowSpec::height},
            {"minWidth", &WindowSpec::minWidth}, {"minHeight", &WindowSpec::minHeight}};

        // Параметр окна (не свойство стиля); false — атрибут не из их числа
        // A window parameter (not a style property); false — the attribute is not one of them
        bool readWindowParameter(const beresta::Attribute& attribute, WindowSpec& window)
        {
            if (attribute.name == "title")
            {
                window.title = attribute.value;
                return true;
            }
            if (attribute.name == "resizable")
            {
                window.resizable = style::parseBoolean(attribute.value);
                return true;
            }
            for (const WindowNumber& number : WINDOW_NUMBERS)
            {
                if (attribute.name == number.name)
                {
                    window.*number.member = style::parseNumber(attribute.value);
                    return true;
                }
            }
            return false;
        }

        // <Window>: параметры окна и наследуемые свойства для корня / <Window>: the window parameters and inherited properties for the root
        void readWindow(const beresta::Element& element, BuildContext& context)
        {
            WindowSpec& window = context.document.window;

            for (const beresta::Attribute& attribute : element.getAttributes())
            {
                if (!readWindowParameter(attribute, window))
                    readDeclaration(element, attribute, window.declared.base, context);
            }
        }
    }

    /*
    ** Порядок: сначала ресурсы (тема нужна для подстановки $значений везде), затем
    ** параметры окна, затем единственный корневой элемент содержимого.
    **
    ** The order: resources first (the theme is needed to substitute $values everywhere), then
    ** the window parameters, then the single root content element.
    */
    UiDocument buildDocument(const beresta::Document& dom, const LoadOptions& options)
    {
        UiDocument document;
        BuildContext context{options, document, {}, 0};
        const beresta::Element& window = *dom.getRoot();
        const beresta::Element* child = firstElementChild(window);

        if (child != nullptr && child->getLocalName() == "Resources")
        {
            readResources(*child, context);
            child = nextElementSibling(*child);
        }
        readWindow(window, context);
        if (child != nullptr)
            document.root = buildNode(*child, context);
        document.nodeCount = context.nodeCount;
        document.baseDirectory = options.baseDirectory;
        return document;
    }
}
