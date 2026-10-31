#include "ui/controller.hpp"
#include <functional>
namespace editor
{
    // Единый маршрут для кнопок, меню и горячих клавиш.
    // Buttons, menus and keyboard shortcuts share one route.
    void Controller::bindActions()
    {
        using namespace std::placeholders;
        const char* actions[] = {"newFile", "openFile", "openFolder", "saveFile", "saveFileAs",
            "selectTab", "closeTab", "closeAllTabs", "nextTab", "previousTab", "reopenTab", "closeWindow",
            "confirmSave", "discardClose", "cancelClose", "openEntry", "browseFolder", "toggleSidebar", "togglePanel",
            "showExplorer", "showSearch", "showSourceControl", "showRun", "showExtensions", "showSettings",
            "showAccounts", "showProblems", "showOutput", "showTerminal", "showCommands", "findText", "findNext"};
        for (const char* action : actions)
            app.onAction(action, std::bind(&Controller::enqueue, this, std::string(action), _1));
        app.onAction("documentChanged", std::bind(&Controller::changed, this, _1));
        app.onFileDrop("openDroppedFiles", std::bind(&Controller::dropFiles, this, _1, _2));
    }

    // Операции документа используют стабильный идентификатор.
    // Document operations use a stable identifier.
    void Controller::documentAction(const std::string& action, unsigned id)
    {
        if (action == "newFile") newDocument();
        else if (action == "openFile") openDialog();
        else if (action == "openFolder") openFolder();
        else if (action == "saveFile") saveDocument(active, false);
        else if (action == "saveFileAs") saveDocument(active, true);
        else if (action == "selectTab") select(id);
        else if (action == "closeTab") requestClose(id);
        else if (action == "closeAllTabs") closeAll(false);
        else if (action == "closeWindow") closeAll(true);
        else if (action == "nextTab") cycle(1);
        else if (action == "previousTab") cycle(-1);
        else if (action == "reopenTab") reopen();
    }

    // Средняя кнопка закрывает вкладку; событие не превращается в выбор.
    // The middle button closes a tab without turning into a selection event.
    void Controller::pointer(gran::ui::Node& node, gran::UiEvent& event)
    {
        if (event.button != gran::platform::MouseButton::MIDDLE && event.button != gran::platform::MouseButton::LEFT)
            return;
        event.preventDefault();
        event.stopPropagation();
        auto* item = document(nodeDocument(node));
        if (!item) return;
        if (event.button == gran::platform::MouseButton::MIDDLE || event.target == item->close)
            enqueue("closeTab", node);
        else
            enqueue("selectTab", node);
    }
}
