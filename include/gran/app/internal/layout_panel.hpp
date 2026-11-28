#ifndef GRAN_APP_INTERNAL_LAYOUT_PANEL_HPP
#define GRAN_APP_INTERNAL_LAYOUT_PANEL_HPP

#include "gran/app/internal/app_state.hpp"
#include "gran/core/i18n.hpp"
#include "beresta.hpp"

namespace gran::app::panel
{
    using Element = beresta::Element;
    using Message = i18n::Message;

    Element& child(Element& parent, std::string tag);
    Element& field(Element& section, const char* id, Message label, bool choice);
    void option(Element& combo, const std::string& id, const std::string& label);
    void button(Element& section, const char* action, Message label);
    Element& page(Element& tabs, Message label);
    void layoutPage(Element& tabs, const markup::UiDocument& doc, const ui::Node& target);
    void palettePage(Element& tabs, const markup::UiDocument& doc);
    bool stylePage(Element& tabs, const markup::UiDocument& doc, bool local);
    void profilePage(Element& tabs, Application::State& state);
    enum class Choice { COMMAND, TEMPLATE, TAB, TOKEN };
    bool resourceChoices(Element& combo, const markup::UiDocument& doc, Choice choice);
    const Element* paletteEntry(const markup::UiDocument& doc, const std::string& name);
    std::string creationXml(const markup::UiDocument& doc, const ui::Node& parent);
    void updateCreationFields(AppWindow& win, ui::Node& root, const Element& prototype);
    ui::Node* creationParent(AppWindow& win);
    bool profileElement(const ui::Node& node, const markup::UiDocument& doc);
    std::string newElementId(AppWindow& win);
    void createElement(AppWindow& win, ui::Node& root);
    std::string value(const ui::Node& panel, const std::string& id);
    bool profileAction(AppWindow& win, const std::string& action);
    void populateParameters(AppWindow& win, ui::Node& root);
    markup::TokenMap readParameters(ui::Node& panel);
}

#endif
