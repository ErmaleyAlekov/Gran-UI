#include "gran/markup/internal/components.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <string>
#include <utility>

namespace gran::markup::internal
{
    namespace
    {
        // Пространство имён XInclude: такой элемент вне Resources — не экземпляр / The XInclude namespace: such an element outside Resources is not an instance
        constexpr std::string_view XINCLUDE_NAMESPACE = "http://www.w3.org/2001/XInclude";

        // Объявление по пространству имён и локальному имени экземпляра / The declaration by the instance namespace and local name
        const ComponentSpec& findComponent(const beresta::Element& instance, const BuildContext& context)
        {
            std::string namespaceUri = instance.getNamespaceURI().value_or(std::string());
            const ComponentSet* set = context.document.components.get();

            if (namespaceUri == XINCLUDE_NAMESPACE)
                throwElementError(instance, i18n::format(i18n::Message::MARKUP_INCLUDE_PLACE));
            for (std::size_t library = 0; set != nullptr && library < set->libraries.size(); ++library)
            {
                if (set->libraries[library].namespaceUri != namespaceUri)
                    continue;
                for (const ComponentSpec& spec : set->components)
                {
                    if (spec.library == library && spec.name == instance.getLocalName())
                        return spec;
                }
                throwElementError(instance, i18n::format(i18n::Message::MARKUP_UNKNOWN_COMPONENT,
                    {instance.getLocalName(), namespaceUri}));
            }
            throwElementError(instance, i18n::format(i18n::Message::MARKUP_NO_LIBRARY, {instance.getName(), namespaceUri}));
        }

        // Компонент уже разворачивается выше по телам — бесконечная развёртка / The component is already being expanded higher up the bodies — an endless expansion
        void checkRecursion(const ComponentSpec& spec, const beresta::Element& instance, const BuildContext& context)
        {
            for (const ComponentScope* scope = context.scope; scope != nullptr; scope = scope->outer)
            {
                if (scope->spec == &spec)
                    throwElementError(instance, i18n::format(i18n::Message::MARKUP_COMPONENT_RECURSION, {spec.name}));
            }
        }

        // Ошибка внутри тела указывает на экземпляр: строка и столбец тела — в тексте / An error inside the body points at the instance: the body line and column are in the text
        [[noreturn]] void rethrowAtInstance(const MarkupException& error, const beresta::Element& instance, const ComponentSpec& spec)
        {
            const beresta::SourceLocation& location = instance.getSourceLocation();

            throw MarkupException(error.getCode(), i18n::format(i18n::Message::MARKUP_IN_COMPONENT,
                {std::to_string(location.line), std::to_string(location.column), spec.name, error.getMessage()}),
                location.line, location.column);
        }
    }

    /*
    ** Своего узла у экземпляра нет: корень тела становится им — ссылается на элемент экземпляра (живая
    ** правка его атрибутов перестраивает развёртку) и держит копию тела, на которую ссылаются узлы тела.
    **
    ** The instance has no node of its own: the body root becomes it — it refers to the instance element (a live
    ** edit of its attributes rebuilds the expansion) and keeps the body copy the body nodes refer to.
    */
    std::unique_ptr<ui::Node> expandComponent(const beresta::Element& instance, BuildContext& context)
    {
        const ComponentSpec& spec = findComponent(instance, context);
        ComponentScope scope{&spec, &instance, context.scope};
        std::shared_ptr<beresta::Document> shadow;
        std::unique_ptr<ui::Node> node;

        checkRecursion(spec, instance, context);
        checkSlots(instance, spec);
        try
        {
            shadow = makeShadow(instance, makeSubstitution(instance, spec, context));
            context.scope = &scope;
            node = buildNode(*shadow->getRoot(), context);
        }
        catch (const MarkupException& error)
        {
            rethrowAtInstance(error, instance, spec);
        }
        context.scope = scope.outer;
        node->domElement = &instance;
        node->line = instance.getSourceLocation().line;
        node->column = instance.getSourceLocation().column;
        node->expansions.push_back(std::move(shadow));
        return node;
    }
}
