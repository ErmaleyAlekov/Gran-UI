#include "gran/app/internal/layout_panel.hpp"
#include "gran/markup/customization.hpp"

namespace gran::app
{
    // Поля адресуются относительно диалога, выбранный объект задаётся жестом.
    // Fields are scoped to the dialog and the gesture identifies the edited object.
    static std::string inspectorXml(AppWindow& win, const ui::Node* target)
    {
        using namespace panel;
        Element dialog("Dialog");
        dialog.setAttribute("width", "90%");
        dialog.setAttribute("maxWidth", "480");
        dialog.setAttribute("height", "85%");
        const auto title = target ? i18n::format(Message::LAYOUT_TARGET) + " — " + target->id : i18n::format(Message::LAYOUT_GLOBAL);
        dialog.setAttribute("accessibleName", title);
        child(dialog, "Text").setTextContent(title);
        auto& tabs = child(dialog, "Tabs");
        tabs.setAttribute("grow", "1");
        if (target)
        {
            layoutPage(tabs, win.document, *target);
            palettePage(tabs, win.document);
        }
        if (!stylePage(tabs, win.document, target != nullptr) && !target) return {};
        auto& section = child(child(dialog, "StatusBar"), "StatusSection");
        section.setAttribute("width", "100%");
        section.setAttribute("orientation", "vertical");
        section.setAttribute("align", "stretch");
        section.setAttribute("textWrap", "wrap");
        child(section, "Text").setAttribute("id", "feedback");
        button(dialog, "dismiss", Message::WINDOW_CLOSE);
        std::string xml;
        dialog.serialize(xml);
        return xml;
    }

    // Оба инструмента используют один слой и один путь возврата фокуса.
    // Both tools share the same layer and focus restoration path.
    static ui::Node& mountInspector(AppWindow& win, const std::string& xml)
    {
        auto& app = *win.shared.owner;
        if (auto* previous = layoutInspector(win))
        {
            closeLayoutInspector(win);
            app.remove(*previous);
        }
        auto node = markup::buildFragment(xml, win.document);
        ui::applyInitialStates(*node);
        node->layoutInspector = true;
        win.shared.editDrop.clear();
        win.shared.editDragging = false;
        return ui::appendChild(*win.document.root, std::move(node));
    }

    // Создаём только в области, которую пользователь выбрал до вызова команды.
    // Create only in the region selected before invoking the command.
    void openLayoutCreator(AppWindow& win)
    {
        if (!win.shared.editMode || win.shared.editSelection.empty() || win.shared.windows.front().get() != &win) return;
        auto* parent = panel::creationParent(win);
        if (!parent) return;
        auto& dialog = mountInspector(win, panel::creationXml(win.document, *parent));
        panel::populateParameters(win, dialog);
        win.shared.owner->showDialog(dialog);
    }

    // Построение без DOM-родителя исключает инструменты из сериализации профиля.
    // Building without a DOM parent excludes editing controls from profile serialization.
    void openLayoutInspector(AppWindow& win, ui::Node* target)
    {
        auto& app = *win.shared.owner;
        const auto xml = inspectorXml(win, target);
        if (xml.empty()) return;
        auto& dialog = mountInspector(win, xml);
        auto* destination = ui::findNode(dialog, "destination");
        const auto* parent = target ? target->parent : nullptr;
        if (parent && !parent->id.empty()) app.selectOption(*destination, parent->id);
        win.shared.editSelection = target ? target->id : "";
        win.shared.editDrop.clear();
        win.shared.editDragging = false;
        app.showDialog(dialog);
    }
}
