#include "gran/markup/internal/components.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

#include <utility>

namespace gran::markup::internal
{
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
