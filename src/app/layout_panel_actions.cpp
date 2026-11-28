#include "gran/app/internal/layout_panel.hpp"
#include "gran/app/commands.hpp"
#include "gran/core/gran_exception.hpp"

namespace gran::app::panel
{
    // У списка читаем значение пункта, у поля ввода — набранный текст.
    // Read the item value from a list and the entered text from an input.
    std::string value(const ui::Node& root, const std::string& id)
    {
        const auto* field = ui::findNode(const_cast<ui::Node&>(root), id);
        if (!field)
            return {};
        return field->kind == ui::ElementKind::COMBO_BOX ? field->content.option : field->content.text;
    }

    // Источник команды находится в палитре того же окна.
    // Resolve the selected command from this window's palette.
    static void commandEdit(Application& app, ui::Node& root, const std::string& action, const std::string& target)
    {
        const bool step = action == "step";
        const auto source = value(root, step ? "action" : "source");
        if (source.compare(0, 2, "c:") != 0)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(Message::LAYOUT_SOURCE));
        const auto command = source.substr(2);
        if (action == "rebind") return app.editLayout("Rebind", {{"id", target}, {"command", command}});
        if (action == "shortcut") return app.editLayout("Shortcut", {{"command", command}, {"key", value(root, "shortcut")}});
        if (step)
        {
            auto previous = value(root, "sequence");
            writeProperty(app, *ui::findNode(root, "sequence"), "text", previous.empty() ? command : previous + " " + command);
        }
    }

    // Кнопки панели вызывают те же операции, что и внешний интерфейс приложения.
    // Panel buttons use the same operations as the public application interface.
    void edit(AppWindow& win, ui::Node& root, const std::string& action)
    {
        auto& app = *win.shared.owner;
        const auto& target = win.shared.editSelection;
        if (action == "move")
        {
            markup::TokenMap placement{{"id", target}, {"parent", value(root, "destination")}};
            auto before = value(root, "before");
            if (!before.empty() && before != "-") placement["before"] = before;
            return app.editLayout("Move", placement);
        }
        if (action == "resetScope") return app.resetProfile(target);
        if (action == "hide") return app.editLayout("Hide", {{"id", target}});
        if (action == "resize") return app.editLayout("Attribute", {{"id", target},
            {"name", value(root, "dimension")}, {"value", value(root, "length")}});
        if (action == "style" || action == "preview")
        {
            markup::TokenMap values{{"name", value(root, "token")}, {"value", value(root, "value")}};
            if (value(root, "scope") != "global") values["id"] = target;
            if (action == "preview") return previewStyle(win.shared, values);
            return app.editLayout("Token", values);
        }
        commandEdit(app, root, action, target);
    }
}

namespace gran::app
{
    // Сообщение об отказе остаётся в строке состояния панели, доступной диктору.
    // A rejection message remains in the panel status bar for the screen reader.
    void layoutPanelAction(AppWindow& win, ui::Node& source, const std::string& action)
    {
        auto* root = win.document.root.get();
        const bool profiles = root == win.shared.layoutPanel;
        if (!profiles) root = layoutInspector(win);
        if (!profiles && (!win.shared.editMode || !root || !root->visible || !input::within(&source, *root))) return;
        std::string message;
        try
        {
            if (profiles && !panel::profileAction(win, action)) return;
            if (!profiles)
            {
                if (action == "dismiss") return closeLayoutInspector(win);
                if (action == "parameters") return panel::populateParameters(win, *root);
                if (action == "create")
                {
                    panel::createElement(win, *root);
                    return closeLayoutInspector(win);
                }
                panel::edit(win, *root, action);
            }
            message = i18n::format(i18n::Message::LAYOUT_OK);
        }
        catch (const std::exception& error)
        {
            auto& feedback = *ui::findNode(*root, "feedback");
            writeProperty(*win.shared.owner, feedback, "text", error.what());
            if (action != "preview" && win.window && feedback.parent->kind == ui::ElementKind::STATUS_SECTION)
                win.window->accessAnnounce(*feedback.parent);
            return;
        }
        writeProperty(*win.shared.owner, *ui::findNode(*root, "feedback"), "text", message);
    }
}
