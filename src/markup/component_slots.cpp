#include "gran/markup/internal/components.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <algorithm>

namespace gran::markup::internal
{
    namespace
    {
        // Слот, куда идёт ребёнок экземпляра: slot="имя" или безымянный ("") / The slot an instance child goes to: slot="name" or the unnamed one ("")
        std::string slotOf(const beresta::Element& child)
        {
            const std::string* name = findAttribute(child, "slot");

            if (name == nullptr || isSlotElement(child))
                return std::string();
            return *name;
        }

        /*
        ** Дети экземпляра для слота name строятся в области outer: их выражения, id и вложенные <Slot>
        ** (пересылка слота внешнего компонента) принадлежат тому, кто написал экземпляр.
        **
        ** The instance children for the slot name are built in the outer scope: their expressions, ids and nested <Slot>
        ** (forwarding a slot of the outer component) belong to whoever wrote the instance.
        */
        bool buildAssigned(const ComponentScope& scope, const std::string& name, ui::Node& node, BuildContext& context)
        {
            bool assigned = false;

            context.scope = scope.outer;
            for (const beresta::Element* child = firstElementChild(*scope.instance); child != nullptr;
                 child = nextElementSibling(*child))
            {
                if ((!isUiElement(*child) && !isSlotElement(*child)) || slotOf(*child) != name)
                    continue;
                assigned = true;
                if (isSlotElement(*child))
                    buildSlot(*child, node, context);
                else
                    ui::appendChild(node, buildNode(*child, context));
            }
            context.scope = &scope;
            return assigned;
        }
    }

    // Все <Slot> поддерева, в том числе в детях вложенных экземпляров (так слот пересылают дальше) / All the subtree <Slot>s, including the children of nested instances (this is how a slot is forwarded further)
    void collectSlots(const beresta::Element& element, ComponentSpec& spec)
    {
        for (const beresta::Element* child = firstElementChild(element); child != nullptr; child = nextElementSibling(*child))
        {
            const std::string* name = findAttribute(*child, "name");
            if (isSlotElement(*child) && name == nullptr)
                spec.defaultSlot = true;
            else if (isSlotElement(*child))
                spec.slots.push_back(*name);
            collectSlots(*child, spec);
        }
    }

    // До построения: ошибка у ребёнка точнее, чем у построенного наполовину дерева / Before building: an error at the child is more precise than at a half-built tree
    void checkSlots(const beresta::Element& instance, const ComponentSpec& spec)
    {
        for (const beresta::Element* child = firstElementChild(instance); child != nullptr; child = nextElementSibling(*child))
        {
            std::string name = slotOf(*child);
            if (!isUiElement(*child) && !isSlotElement(*child))
                continue;
            if (name.empty() && !spec.defaultSlot)
                throwElementError(*child, i18n::format(i18n::Message::MARKUP_NO_DEFAULT_SLOT, {spec.name}));
            if (!name.empty() && std::find(spec.slots.begin(), spec.slots.end(), name) == spec.slots.end())
                throwAttributeError(*child, "slot", i18n::format(i18n::Message::MARKUP_UNKNOWN_SLOT, {spec.name, name}));
        }
    }

    // Пустой слот показывает своё запасное содержимое — оно из тела, область та же / An empty slot shows its fallback content — it is from the body, the scope is the same
    void buildSlot(const beresta::Element& slot, ui::Node& node, BuildContext& context)
    {
        const std::string* name = findAttribute(slot, "name");
        std::string slotName;

        if (context.scope == nullptr)
            throwElementError(slot, i18n::format(i18n::Message::MARKUP_SLOT_OUTSIDE));
        if (name != nullptr)
            slotName = *name;
        if (!buildAssigned(*context.scope, slotName, node, context))
            buildChildren(slot, node, context);
    }
}
