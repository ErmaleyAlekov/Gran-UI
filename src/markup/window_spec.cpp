#include "gran/markup/internal/build_context.hpp"
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

        /*
        ** Перечислимые параметры окна: titleBar, kind, position. Значения проверила схема,
        ** поэтому parseKeyword не встречает чужих слов.
        **
        ** The enumerated window parameters: titleBar, kind, position. The schema has checked the values,
        ** so parseKeyword meets no foreign words.
        */
        bool readWindowKeyword(const beresta::Attribute& attribute, WindowSpec& window)
        {
            static const style::Keyword TITLES[] = {{"system", 0}, {"custom", 1}};
            static const style::Keyword KINDS[] = {{"normal", 0}, {"dialog", 1}, {"tool", 2}};
            static const style::Keyword POSITIONS[] = {{"default", 0}, {"center", 1}, {"centerOwner", 2}};

            if (attribute.name == "titleBar")
                window.titleBar = static_cast<platform::TitleBar>(style::parseKeyword(attribute.value, TITLES));
            else if (attribute.name == "kind")
                window.kind = static_cast<platform::WindowKind>(style::parseKeyword(attribute.value, KINDS));
            else if (attribute.name == "position")
                window.position = static_cast<platform::WindowPosition>(
                    style::parseKeyword(attribute.value, POSITIONS));
            else
                return false;
            return true;
        }

        // Параметр окна (не свойство стиля); false — атрибут не из их числа
        // A window parameter (not a style property); false — the attribute is not one of them
        bool readWindowChoice(const beresta::Attribute& attribute, WindowSpec& window)
        {
            if (readWindowKeyword(attribute, window))
                return true;
            if (attribute.name == "title")
                window.title = attribute.value;
            else if (attribute.name == "onClose")
                window.onClose = attribute.value;
            else if (attribute.name == "layout")
                window.layout = attribute.value;
            else if (attribute.name == "resizable")
                window.resizable = style::parseBoolean(attribute.value);
            else
                return false;
            return true;
        }

        // Числовые размеры хранятся отдельно от параметров режима окна.
        // Numeric dimensions are stored separately from window mode parameters.
        bool readWindowParameter(const beresta::Attribute& attribute, WindowSpec& window)
        {
            if (readWindowChoice(attribute, window))
                return true;
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
