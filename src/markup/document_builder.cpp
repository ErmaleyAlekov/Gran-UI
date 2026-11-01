#include "gran/markup/internal/components.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

#include <utility>

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
        bool readWindowChoice(const beresta::Attribute& attribute, WindowSpec& window)
        {
            if (attribute.name == "titleBar")
            {
                static const style::Keyword TITLES[] = {{"system", 0}, {"custom", 1}};
                window.titleBar = static_cast<platform::TitleBar>(style::parseKeyword(attribute.value, TITLES, 2));
                return true;
            }
            if (attribute.name == "title" || attribute.name == "onClose")
            {
                if (attribute.name == "title")
                    window.title = attribute.value;
                else
                    window.onClose = attribute.value;
                return true;
            }
            if (attribute.name == "resizable")
            {
                window.resizable = style::parseBoolean(attribute.value);
                return true;
            }
            return false;
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
    ** Порядок: сначала ресурсы (тема нужна для подстановки $значений везде) и библиотеки компонентов,
    ** затем параметры окна, затем единственный корневой элемент содержимого и его ссылки на команды.
    ** Схема знает и корень <Library> (файлы библиотек), поэтому окно проверяется здесь.
    **
    ** The order: resources first (the theme is needed to substitute $values everywhere) and the component libraries,
    ** then the window parameters, then the single root content element and its command references.
    ** The schema knows the <Library> root too (the library files), so the window is checked here.
    */
    UiDocument buildDocument(std::shared_ptr<beresta::Document> dom, const LoadOptions& options)
    {
        UiDocument document;
        BuildContext context{options, document, {}, 0};
        const beresta::Element& window = *dom->getRoot();
        const beresta::Element* child = firstElementChild(window);

        if (window.getLocalName() != "Window")
            throwElementError(window, i18n::format(i18n::Message::MARKUP_ROOT_NOT_WINDOW, {window.getName()}));
        if (child != nullptr && child->getLocalName() == "Resources")
        {
            readResources(*child, context);
            loadComponents(*child, context);
            child = nextElementSibling(*child);
        }
        readWindow(window, context);
        if (child != nullptr)
            document.root = buildNode(*child, context);
        if (document.root)
            ui::bindCommandTree(*document.root, document.commands);
        document.nodeCount = context.nodeCount;
        document.baseDirectory = options.baseDirectory;
        document.tokens = std::move(context.tokens);
        document.dom = std::move(dom);
        return document;
    }
}
