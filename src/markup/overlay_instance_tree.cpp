#include "gran/markup/internal/components.hpp"
#include "gran/markup/internal/overlay.hpp"

namespace gran::markup::internal
{
    // Область уникальности XML включает ресурсы, шаблоны и скрытые элементы.
    // XML uniqueness includes resources, prototypes and hidden elements.
    void collectMarkupIds(const beresta::Element& element, std::set<std::string>& ids)
    {
        if (auto id = element.getAttribute("id")) ids.insert(*id);
        for (auto* child = firstElementChild(element); child; child = nextElementSibling(*child))
            collectMarkupIds(*child, ids);
    }

    // Имена и локальные ссылки готового шаблона получают ту же область, что и у компонента.
    // Scope the template's IDs and local references just like component body IDs.
    std::unique_ptr<beresta::Element> nativePaletteInstance(OverlayContext& context, const beresta::Element& prototype,
        const beresta::Element& op)
    {
        if (firstElementChild(op)) denyOverlay(op, "Param");
        ComponentSpec spec;
        spec.body = &prototype;
        LoadOptions options;
        BuildContext build{options, context.candidate, context.candidate.tokens, 0};
        beresta::Element host("Instance");
        host.setAttribute("id", *findAttribute(op, "id"));
        auto substitution = makeSubstitution(host, spec, build);
        auto shadow = makeShadow(host, substitution);
        return std::make_unique<beresta::Element>(*shadow->getRoot());
    }

    // Следующая операция профиля может адресовать опубликованного потомка по полному id.
    // Later profile operations may address a published descendant by its qualified ID.
    void indexPaletteTree(OverlayContext& context, beresta::Element& node)
    {
        const auto id = node.getAttribute("id");
        if (id && !context.targets.emplace(*id, &node).second) denyOverlay(node, *id);
        for (auto* child = firstElementChild(node); child; child = nextElementSibling(*child))
            if (isUiElement(*child)) indexPaletteTree(context, const_cast<beresta::Element&>(*child));
    }
}
