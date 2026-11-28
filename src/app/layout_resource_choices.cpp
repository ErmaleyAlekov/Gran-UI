#include "gran/app/internal/layout_panel.hpp"
#include "gran/markup/internal/build_context.hpp"

namespace gran::app::panel
{
    // Для полосы вкладок подходят только шаблоны страниц.
    // A tab strip accepts only page templates.
    static bool matches(const Element& item, Choice choice)
    {
        if (choice == Choice::TOKEN) return item.hasAttribute("type");
        if (choice == Choice::COMMAND) return item.getLocalName() == "CommandRef";
        if (item.getLocalName() != "Template") return false;
        const auto* prototype = markup::internal::firstElementChild(item);
        return (prototype->getLocalName() == "Tab" && prototype->getNamespaceURI().value_or("").empty()) == (choice == Choice::TAB);
    }

    // Пользователь выбирает команду по подписи, профиль хранит стабильный ключ.
    // The user picks the command by caption while the profile stores its stable key.
    static std::string commandLabel(const markup::UiDocument& doc, const std::string& id)
    {
        for (const auto& command : doc.commands)
            if (command.id == id && !command.text.empty()) return command.text;
        return id;
    }

    // Список содержит только значения, опубликованные автором текущего окна.
    // The list contains only the values published by the current window's author.
    bool resourceChoices(Element& combo, const markup::UiDocument& doc, Choice choice)
    {
        using namespace markup::internal;
        auto* resources = firstElementChild(*doc.dom->getRoot());
        if (!resources || resources->getLocalName() != "Resources") return false;
        bool added = false;
        const bool command = choice == Choice::COMMAND;
        const bool token = choice == Choice::TOKEN;
        for (auto* section = firstElementChild(*resources); section; section = nextElementSibling(*section))
        {
            if (section->getLocalName() != (token ? "Theme" : "Palette")) continue;
            for (auto* item = firstElementChild(*section); item; item = nextElementSibling(*item))
            {
                if (!matches(*item, choice)) continue;
                const auto name = item->getAttribute(command ? "ref" : "name");
                if (!name) continue;
                const auto id = token ? *name : (command ? "c:" : "t:") + *name;
                if (!added) combo.setAttribute("value", id);
                option(combo, id, command ? commandLabel(doc, *name) : *name);
                added = true;
            }
        }
        return added;
    }
}
