#include "gran/app/internal/layout_panel.hpp"
#include "gran/markup/internal/components.hpp"

#include <sstream>

namespace gran::app::panel
{
    // Шаблон выбирается только в палитре авторской разметки.
    // Resolve the template exclusively in the author's palette.
    const Element* paletteEntry(const markup::UiDocument& doc, const std::string& name)
    {
        using namespace markup::internal;
        auto* resources = firstElementChild(*doc.dom->getRoot());
        if (!resources || resources->getLocalName() != "Resources") return nullptr;
        for (auto* section = firstElementChild(*resources); section; section = nextElementSibling(*section))
            if (section->getLocalName() == "Palette")
                for (auto* item = firstElementChild(*section); item; item = nextElementSibling(*item))
                    if (item->getLocalName() == "Template" && item->getAttribute("name") == name)
                        return item;
        return nullptr;
    }

    // При смене шаблона поля получают имена и начальные значения из прототипа.
    // When the template changes, populate fields from the prototype's names and initial values.
    void populateParameters(AppWindow& win, ui::Node& root)
    {
        auto& app = *win.shared.owner;
        auto& fields = *ui::findNode(root, "parameters");
        while (!fields.children.empty()) app.remove(*fields.children.back());
        auto selection = value(root, "source");
        if (selection.compare(0, 2, "t:") != 0) return;
        auto* entry = paletteEntry(win.document, selection.substr(2));
        if (!entry) return;
        auto* instance = markup::internal::firstElementChild(*entry);
        if (ui::findNode(root, "actions")) updateCreationFields(win, root, *instance);
        std::istringstream names(entry->getAttribute("parameters").value_or(""));
        for (std::string key; names >> key;)
        {
            Element row("Stack");
            child(row, "Text").setTextContent(key);
            auto& input = child(row, "Input");
            input.setAttribute("accessibleName", key);
            input.setAttribute("value", instance->getAttribute(key).value_or(""));
            std::string xml;
            row.serialize(xml);
            app.appendMarkup(fields, xml);
        }
    }

    // Пустое поле сохраняет значение параметра, заданное разработчиком.
    // An empty field retains the parameter supplied by the developer.
    markup::TokenMap readParameters(ui::Node& root)
    {
        markup::TokenMap parameters;
        auto& fields = *ui::findNode(root, "parameters");
        for (const auto& row : fields.children)
        {
            const auto& input = *row->children.back();
            if (!input.content.text.empty())
                parameters[input.content.accessibleName] = input.content.text;
        }
        return parameters;
    }
}
