#include "gran/markup/internal/overlay.hpp"

#include <sstream>
#include <set>

namespace gran::markup::internal
{
    // Ищем запись только в указанной секции ресурсов авторской разметки.
    // Look up an entry only in the specified resource section of the author's markup.
    const beresta::Element* overlayResource(const OverlayContext& context, std::string_view group,
        std::string_view tag, std::string_view key, std::string_view value)
    {
        auto* resources = firstElementChild(*context.candidate.dom->getRoot());
        if (!resources || resources->getLocalName() != "Resources")
            return nullptr;
        for (auto* section = firstElementChild(*resources); section; section = nextElementSibling(*section))
        {
            if (section->getLocalName() != group)
                continue;
            for (auto* entry = firstElementChild(*section); entry; entry = nextElementSibling(*entry))
                if (entry->getLocalName() == tag && entry->getAttribute(key) == value)
                    return entry;
        }
        return nullptr;
    }

    namespace
    {
        // Копия экземпляра сохраняет ближайшие объявления префиксов исходного шаблона.
        // The instance copy retains the prototype's nearest prefix declarations.
        void copyNamespaces(const beresta::Element& prototype, beresta::Element& instance)
        {
            for (auto* current = &prototype; current; current = dynamic_cast<const beresta::Element*>(current->getParent()))
                for (const auto& attr : current->getAttributes())
                    if ((attr.name == "xmlns" || attr.name.rfind("xmlns:", 0) == 0) && !instance.hasAttribute(attr.name))
                        instance.setAttribute(attr.name, attr.value);
        }

        // Каждый открытый параметр может быть задан одной операцией лишь один раз.
        // Each exposed parameter may be set only once by an operation.
        void setParameters(const beresta::Element& entry, const beresta::Element& op, beresta::Element& instance)
        {
            std::istringstream names(entry.getAttribute("parameters").value_or(""));
            std::set<std::string> allowed;
            for (std::string name; names >> name;) allowed.insert(name);
            for (auto* param = firstElementChild(op); param; param = nextElementSibling(*param))
            {
                const auto& key = *findAttribute(*param, "name");
                const auto& value = *findAttribute(*param, "value");
                if (!allowed.erase(key) || value.find_first_of("{}$") != std::string::npos)
                    denyOverlay(*param, key);
                instance.setAttribute(key, value);
            }
        }
    }

    // Новый узел происходит из зарегистрированной команды или объявленного шаблона.
    // Create the node from a registered command or a declared template.
    std::unique_ptr<beresta::Element> paletteInstance(OverlayContext& context, const beresta::Element& op)
    {
        if (!op.hasAttribute("template"))
        {
            const auto* command = findAttribute(op, "command");
            if (command && !overlayResource(context, "Palette", "CommandRef", "ref", *command))
                denyOverlay(op, *command);
            auto button = std::make_unique<beresta::Element>("Button");
            button->setAttribute("command", command ? *command : addSequence(context, op, *findAttribute(op, "text")));
            return button;
        }
        const auto& name = *findAttribute(op, "template");
        const auto* entry = overlayResource(context, "Palette", "Template", "name", name);
        if (!entry || !firstElementChild(*entry))
            denyOverlay(op, name);
        const auto& prototype = *firstElementChild(*entry);
        if (prototype.getNamespaceURI().value_or("").empty())
            return nativePaletteInstance(context, prototype, op);
        auto instance = std::make_unique<beresta::Element>(prototype);
        copyNamespaces(prototype, *instance);
        setParameters(*entry, op, *instance);
        return instance;
    }
}
