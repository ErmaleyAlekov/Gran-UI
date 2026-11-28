#include "gran/app/internal/layout_panel.hpp"
#include "gran/app/commands.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/markup/customization.hpp"
#include "gran/markup/internal/build_context.hpp"

namespace gran::app::panel
{
    // Новая кнопка получает шаги из опубликованной палитры в порядке выбора.
    // A new button takes its steps from the published palette in selection order.
    static void actionFields(Element& form, const markup::UiDocument& doc)
    {
        auto& actions = child(form, "Stack");
        actions.setAttribute("id", "actions");
        actions.setAttribute("visible", "false");
        resourceChoices(field(actions, "action", Message::LAYOUT_ACTIONS, true), doc, Choice::COMMAND);
        button(actions, "step", Message::LAYOUT_ADD_STEP);
        field(actions, "sequence", Message::LAYOUT_MACRO, false);
        field(actions, "caption", Message::LAYOUT_CAPTION, false);
    }

    // Стиль задаётся опубликованным токеном только для нового экземпляра.
    // A published token styles only the new instance.
    static void styleFields(Element& form, const markup::UiDocument& doc)
    {
        auto& styles = child(form, "Stack");
        styles.setAttribute("id", "styles");
        styles.setAttribute("visible", "false");
        resourceChoices(field(styles, "token", Message::LAYOUT_TOKEN, true), doc, Choice::TOKEN);
        field(styles, "value", Message::LAYOUT_VALUE, false);
    }

    // Место вставки доступно только для чтения на протяжении всего диалога.
    // The insertion destination is read-only throughout the dialog.
    std::string creationXml(const markup::UiDocument& doc, const ui::Node& parent)
    {
        Element dialog("Dialog");
        dialog.setAttribute("width", "90%");
        dialog.setAttribute("maxWidth", "480");
        dialog.setAttribute("height", "85%");
        const auto title = i18n::format(Message::LAYOUT_CREATE) + " — " + parent.id;
        dialog.setAttribute("accessibleName", title);
        child(dialog, "Text").setTextContent(title);
        auto& scroll = child(dialog, "Scroll");
        scroll.setAttribute("grow", "1");
        auto& form = child(scroll, "Stack");
        auto& source = field(form, "source", Message::LAYOUT_SOURCE, true);
        const bool available = resourceChoices(source, doc, parent.kind == ui::ElementKind::TABS ? Choice::TAB : Choice::TEMPLATE);
        source.setAttribute("onChange", "gran:layout:parameters");
        child(form, "Stack").setAttribute("id", "parameters");
        actionFields(form, doc);
        styleFields(form, doc);
        auto& feedback = child(dialog, "Text");
        feedback.setAttribute("id", "feedback");
        if (available) button(dialog, "create", Message::LAYOUT_CREATE);
        else feedback.setTextContent(i18n::format(Message::LAYOUT_NO_TEMPLATES));
        button(dialog, "dismiss", Message::WINDOW_CLOSE);
        std::string xml;
        dialog.serialize(xml);
        return xml;
    }

    // Без явного списка прав экземпляр наследует разрешения выбранного контейнера.
    // Without explicit permissions an instance inherits the selected container's policy.
    void updateCreationFields(AppWindow& win, ui::Node& root, const Element& prototype)
    {
        using Right = markup::Customization;
        auto* parent = ui::findNode(*win.document.root, win.shared.editSelection);
        if (!parent || !parent->domElement) return;
        const auto& policy = prototype.hasAttribute("customizable") ? prototype : *parent->domElement;
        const bool commands = !value(root, "action").empty() && prototype.getName() == "Button"
            && prototype.getNamespaceURI().value_or("").empty() && !prototype.hasAttribute("windowRole")
            && markup::permitsCustomization(policy, Right::COMMAND);
        auto& app = *win.shared.owner;
        writeProperty(app, *ui::findNode(root, "actions"), "visible", commands ? "true" : "false");
        writeProperty(app, *ui::findNode(root, "styles"), "visible",
            !value(root, "token").empty() && markup::permitsCustomization(policy, Right::STYLE) ? "true" : "false");
        writeProperty(app, *ui::findNode(root, "sequence"), "text", "");
        writeProperty(app, *ui::findNode(root, "value"), "text", "");
        writeProperty(app, *ui::findNode(root, "caption"), "text", prototype.getAttribute("text").value_or(""));
    }

    // Параметры, стиль и команда публикуются одной проверяемой операцией профиля.
    // Parameters, style and command are committed as a single validated profile operation.
    void createElement(AppWindow& win, ui::Node& root)
    {
        const auto source = value(root, "source");
        if (!creationParent(win) || source.compare(0, 2, "t:") != 0)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(Message::LAYOUT_SOURCE));
        auto& app = *win.shared.owner;
        markup::TokenMap attributes{{"parent", win.shared.editSelection}, {"template", source.substr(2)}};
        if (ui::findNode(root, "actions")->visible)
        {
            const auto steps = value(root, "sequence");
            const auto caption = value(root, "caption");
            if (!steps.empty()) attributes["sequence"] = steps;
            const auto* entry = paletteEntry(win.document, attributes.at("template"));
            if (!entry) throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(Message::LAYOUT_SOURCE));
            const auto* prototype = markup::internal::firstElementChild(*entry);
            if (caption != prototype->getAttribute("text").value_or("")) attributes["text"] = caption;
        }
        if (ui::findNode(root, "styles")->visible && !value(root, "value").empty())
        {
            attributes["token"] = value(root, "token");
            attributes["value"] = value(root, "value");
        }
        attributes["id"] = newElementId(win);
        app.editLayout("Insert", attributes, readParameters(root));
        win.shared.editSelection = attributes["id"];
    }
}
