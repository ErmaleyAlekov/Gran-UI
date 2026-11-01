#include "gran/markup/internal/components.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <string>
#include <utility>
#include <vector>

namespace gran::markup::internal
{
    namespace
    {
        // Стиль и его элемент: basedOn связывается после чтения всех стилей уровня / A style and its element: basedOn is linked after all the level styles are read
        using LocalStyle = std::pair<style::StyleRule*, const beresta::Element*>;

        /*
        ** <Style> детей owner — в набор документа под именем prefix + имя («_0-Title» у библиотеки,
        ** «_0.Card.Frame» у компонента: точка и дефис не дают им совпасть), names узнаёт короткое имя.
        **
        ** The <Style> children of owner go into the document set under the name prefix + name ("_0-Title" for a library,
        ** "_0.Card.Frame" for a component: the dot and the hyphen keep them apart), names learns the short name.
        */
        void readStyles(const beresta::Element& owner, const std::string& prefix, StyleNames& names,
            std::vector<LocalStyle>& pending, BuildContext& context)
        {
            for (const beresta::Element* child = firstElementChild(owner); child != nullptr; child = nextElementSibling(*child))
            {
                if (child->getLocalName() != "Style")
                    continue;
                const std::string& name = *findAttribute(*child, "name");
                style::StyleRule& rule = context.document.styles.add(prefix + name);
                readDeclarations(*child, rule.block.base, context);
                readStates(*child, rule.block, context);
                names[name] = prefix + name;
                pending.emplace_back(&rule, child);
            }
        }

        /*
        ** basedOn ищется так же, как style в теле; цепочка длиннее всех стилей документа — цикл
        ** (a → b → a между своими стилями схема не ловит).
        **
        ** basedOn is looked up the same way as style in the body; a chain longer than all the document styles is a cycle
        ** (the schema does not catch a → b → a among the own styles).
        */
        void linkStyles(const std::vector<LocalStyle>& pending, const StyleNames& names, BuildContext& context)
        {
            std::size_t limit = context.document.styles.names().size();

            for (const LocalStyle& local : pending)
            {
                const std::string* basedOn = findAttribute(*local.second, "basedOn");
                if (basedOn == nullptr)
                    continue;
                try
                {
                    local.first->basedOn = context.document.styles.find(resolveStyleName(*basedOn, names, context.document.styles));
                }
                catch (const GranException& error)
                {
                    throwAttributeError(*local.second, "basedOn", error.getMessage());
                }
                std::size_t steps = 0;
                for (const style::StyleRule* rule = local.first; rule != nullptr; rule = rule->basedOn)
                {
                    if (++steps > limit)
                        throwAttributeError(*local.second, "basedOn",
                            i18n::format(i18n::Message::RESOURCE_STYLE_CYCLE, {local.first->name}));
                }
            }
        }

        // <Component name>: параметры, свои стили поверх библиотечных, тело и его слоты / <Component name>: the parameters, the own styles over the library ones, the body and its slots
        void readComponent(const beresta::Element& element, ComponentSet& set, std::size_t library,
            const StyleNames& libraryStyles, BuildContext& context)
        {
            ComponentSpec spec;
            std::vector<LocalStyle> pending;

            spec.name = *findAttribute(element, "name");
            spec.library = library;
            spec.styles = libraryStyles;
            readStyles(element, "_" + std::to_string(library) + "." + spec.name + ".", spec.styles, pending, context);
            for (const beresta::Element* child = firstElementChild(element); child != nullptr; child = nextElementSibling(*child))
            {
                if (child->getLocalName() == "Param")
                    spec.params.push_back(readParam(*child, spec.name));
                else if (child->getLocalName() != "Style")
                    spec.body = child;
            }
            if (isSlotElement(*spec.body))
                throwElementError(*spec.body, i18n::format(i18n::Message::MARKUP_SLOT_ROOT, {spec.name}));
            linkStyles(pending, spec.styles, context);
            collectSlots(*spec.body, spec);
            set.components.push_back(std::move(spec));
        }
    }

    // Сначала стили библиотеки (их видят все её компоненты), затем компоненты по порядку / First the library styles (all its components see them), then the components in order
    void readLibrary(ComponentSet& set, std::size_t library, BuildContext& context)
    {
        const beresta::Element& root = *set.libraries[library].dom->getRoot();
        StyleNames names;
        std::vector<LocalStyle> pending;

        readStyles(root, "_" + std::to_string(library) + "-", names, pending, context);
        linkStyles(pending, names, context);
        for (const beresta::Element* child = firstElementChild(root); child != nullptr; child = nextElementSibling(*child))
        {
            if (child->getLocalName() == "Component")
                readComponent(*child, set, library, names, context);
        }
    }
}
