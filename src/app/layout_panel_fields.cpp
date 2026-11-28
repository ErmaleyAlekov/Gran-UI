#include "gran/app/internal/layout_panel.hpp"

namespace gran::app::panel
{
    // Родитель сразу принимает владение созданным элементом формы.
    // The parent immediately takes ownership of the new form element.
    Element& child(Element& parent, std::string tag)
    {
        return *static_cast<Element*>(parent.appendChild(std::make_unique<Element>(std::move(tag))));
    }

    // Текст подписи и имя для экранного диктора берутся из одного перевода.
    // Use the same translation for the visible label and the screen-reader name.
    Element& field(Element& section, const char* id, Message label, bool choice)
    {
        const auto text = i18n::format(label);
        child(section, "Text").setTextContent(text);
        auto& control = child(section, choice ? "ComboBox" : "Input");
        control.setAttribute("id", id);
        control.setAttribute("accessibleName", text);
        return control;
    }

    // Отображаемая подпись отделена от стабильного значения пункта.
    // Keep the displayed label separate from the stable item value.
    void option(Element& combo, const std::string& id, const std::string& label)
    {
        auto& item = child(combo, "MenuItem");
        item.setAttribute("text", label);
        item.setAttribute("value", id);
    }

    // Действия окна инструментов обрабатывает встроенный диспетчер Gran.
    // The Gran built-in dispatcher handles tools-window actions.
    void button(Element& section, const char* action, Message label)
    {
        auto& control = child(section, "Button");
        control.setAttribute("onClick", std::string("gran:layout:") + action);
        control.setTextContent(i18n::format(label));
    }

    // Каждый раздел формы доступен как обычная страница Tabs.
    // Each section of the form is accessible as an ordinary Tabs page.
    Element& page(Element& tabs, Message label)
    {
        auto& tab = child(tabs, "Tab");
        tab.setAttribute("text", i18n::format(label));
        tab.setAttribute("spacing", "6");
        tab.setAttribute("padding", "8");
        auto& scroll = child(tab, "Scroll");
        scroll.setAttribute("grow", "1");
        return child(scroll, "Stack");
    }
}
