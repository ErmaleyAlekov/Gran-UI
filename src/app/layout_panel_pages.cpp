#include "gran/app/internal/layout_panel.hpp"
#include "gran/markup/customization.hpp"

namespace gran::app::panel
{
    namespace
    {
        // Для наложения пригодны только узлы с собственным id в живом документе.
        // Only nodes with their own ID in the live document can be addressed by the overlay.
        void listNodes(Element& combo, const ui::Node& node, const markup::UiDocument& doc, const ui::Node* exclude = nullptr)
        {
            if (&node == exclude) return;
            if (profileElement(node, doc))
                option(combo, node.id, node.content.accessibleName.empty() ? node.id : node.content.accessibleName);
            for (const auto& child : node.children)
                listNodes(combo, *child, doc, exclude);
        }
    }

    // Клавиатурные поля задают те же источник и место, что и перенос мышью.
    // The keyboard controls identify the same source and position as pointer dragging.
    void layoutPage(Element& tabs, const markup::UiDocument& doc, const ui::Node& target)
    {
        auto& tab = page(tabs, Message::LAYOUT_MOVE);
        auto& parent = field(tab, "destination", Message::LAYOUT_PARENT, true);
        listNodes(parent, *doc.root, doc, &target);
        auto& before = field(tab, "before", Message::LAYOUT_BEFORE, true);
        option(before, "-", i18n::format(Message::LAYOUT_END));
        listNodes(before, *doc.root, doc);
        button(tab, "move", Message::LAYOUT_MOVE);
        button(tab, "hide", Message::LAYOUT_HIDE);
        button(tab, "resetScope", Message::LAYOUT_RESET_SCOPE);
        auto& size = field(tab, "dimension", Message::LAYOUT_SIZE, true);
        option(size, "width", i18n::format(Message::LAYOUT_WIDTH));
        option(size, "height", i18n::format(Message::LAYOUT_HEIGHT));
        field(tab, "length", Message::LAYOUT_VALUE, false);
        button(tab, "resize", Message::LAYOUT_RESIZE);
    }

    // Выбор шаблона обновляет поля параметров; выбор команды открывает операции с сочетаниями.
    // Selecting a template updates its parameter fields; command selection supports shortcut operations.
    void palettePage(Element& tabs, const markup::UiDocument& doc)
    {
        auto& tab = page(tabs, Message::LAYOUT_SOURCE);
        auto& source = field(tab, "source", Message::LAYOUT_SOURCE, true);
        resourceChoices(source, doc, Choice::COMMAND);
        button(tab, "rebind", Message::LAYOUT_REBIND);
        field(tab, "shortcut", Message::LAYOUT_KEY, false);
        button(tab, "shortcut", Message::LAYOUT_ASSIGN);
    }

    // Изменение поля запускает предпросмотр, кнопка фиксирует значение.
    // Editing the field triggers preview; the button commits the value.
    bool stylePage(Element& tabs, const markup::UiDocument& doc, bool local)
    {
        auto& tab = page(tabs, Message::LAYOUT_TOKEN);
        auto& scope = field(tab, "scope", Message::LAYOUT_TARGET, true);
        if (local) option(scope, "local", i18n::format(Message::LAYOUT_TARGET));
        option(scope, "global", i18n::format(Message::LAYOUT_GLOBAL));
        scope.setAttribute("value", local ? "local" : "global");
        const bool available = resourceChoices(field(tab, "token", Message::LAYOUT_TOKEN, true), doc, Choice::TOKEN);
        field(tab, "value", Message::LAYOUT_VALUE, false).setAttribute("onChange", "gran:layout:preview");
        button(tab, "style", Message::LAYOUT_APPLY);
        return available;
    }

}
