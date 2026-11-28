#include "gran/markup/internal/components.hpp"
#include "gran/markup/internal/overlay.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/ui/internal/command_sequence.hpp"

#include "beresta.hpp"

#include <utility>

namespace gran::markup::internal
{
    static void checkSequences(const beresta::Element& resources, const std::vector<ui::Command>& commands)
    {
        std::vector<const ui::Command*> roots;
        std::map<std::string, const ui::Command*> byId;
        for (const auto& command : commands)
        {
            if (!command.sequence.empty()) roots.push_back(&command);
            byId.emplace(command.id, &command);
        }
        ui::SequenceSizes sizes;
        const auto cycle = ui::inspectCommandSequences(roots, [&](const std::string& id)
        {
            const auto found = byId.find(id);
            return found == byId.end() ? nullptr : found->second;
        }, sizes);
        if (!cycle.empty()) throwElementError(resources, i18n::format(i18n::Message::COMMAND_SEQUENCE_CYCLE, {cycle}));
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
            checkSequences(*child, document.commands);
            loadComponents(*child, context);
            validatePalette(*child, context);
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
        checkOverlayDesign(document);
        return document;
    }
}
