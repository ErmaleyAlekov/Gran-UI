#include "ui/controller.hpp"
#include "../core/messages.hpp"
#include "../core/text.hpp"
namespace editor
{
    // Области без backend честно показывают состояние реализации.
    // Areas without a backend report their implementation status honestly.
    void Controller::viewAction(const std::string& action)
    {
        if (action == "toggleSidebar") toggle("sidebar");
        else if (action == "togglePanel") toggle("bottomPanel");
        else if (action == "showExplorer") show("sidebar", true);
        else if (action == "showSearch" || action == "findText") findText();
        else if (action == "findNext") findNext();
        else if (action == "showSourceControl") showPanel("SOURCE CONTROL", message(Message::SOURCE));
        else if (action == "showRun") showPanel("RUN AND DEBUG", message(Message::RUN));
        else if (action == "showExtensions") showPanel("EXTENSIONS", message(Message::EXTENSIONS));
        else if (action == "showSettings") showPanel("SETTINGS", message(Message::SETTINGS));
        else if (action == "showAccounts") showPanel("ACCOUNTS", message(Message::ACCOUNTS));
        else if (action == "showProblems") showPanel("PROBLEMS", message(Message::PROBLEMS));
        else if (action == "showOutput") showPanel("OUTPUT", message(Message::OUTPUT));
        else if (action == "showTerminal") showPanel("TERMINAL", message(Message::TERMINAL));
        else if (action == "showCommands") showPanel("COMMANDS", message(Message::COMMANDS));
    }

    // Панель общая для диагностических представлений. / Diagnostic views share one panel.
    void Controller::showPanel(const std::string& title, const std::string& body)
    {
        show("bottomPanel", true);
        show("findControls", false);
        setText("panelTitle", title);
        setText("panelText", body);
    }

    // Переключатель сохраняет содержимое области. / Toggling preserves the area's content.
    void Controller::toggle(const std::string& id)
    {
        if (auto* node = app.find(id)) show(id, !node->visible);
    }

    // Поле поиска создаётся один раз, его значение переживает смену панели.
    // The search field is created once; its value survives panel changes.
    void Controller::findText()
    {
        showPanel("SEARCH", message(Message::SEARCH));
        if (!app.find("findControls"))
            app.appendMarkup(*app.find("bottomPanel"),
                "<Stack id='findControls' orientation='horizontal' spacing='8' padding='8'>"
                "<Input id='findQuery' grow='1' onSubmit='findNext'/>"
                "<Button text='↓' tooltip='Find next' onClick='findNext'/></Stack>");
        show("findControls", true);
        app.focus(app.find("findQuery"));
    }

    // Поиск идёт вперёд от выделения и заворачивается к началу документа.
    // Search advances from the selection and wraps to the start of the document.
    void Controller::findNext()
    {
        auto* item = document(active);
        auto* query = app.find("findQuery");
        if (!item || !query || query->content.text.empty()) return;
        auto& field = item->page->content;
        auto position = field.text.find(query->content.text, field.caret);
        if (position == std::string::npos) position = field.text.find(query->content.text);
        if (position == std::string::npos)
        {
            setText("panelText", message(Message::NOT_FOUND));
            return;
        }
        field.anchor = position;
        field.caret = position + query->content.text.size();
        app.focus(item->page);
        gran::ui::markDirty(*item->page, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        setText("panelText", cursorLabel(field.text, position));
        updateCursor();
    }
}
