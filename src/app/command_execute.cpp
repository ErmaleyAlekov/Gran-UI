#include "gran/app/commands.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::app
{
    // Команды окна не требуют узла; прочие разрешаются по id непосредственно перед вызовом.
    // Window commands need no node; other commands resolve an id immediately before calling.
    static bool windowCommand(Application& app, const std::string& command)
    {
        if (command == "close") app.close();
        else if (command == "minimize") app.minimize();
        else if (command == "maximize") app.maximize();
        else if (command == "restore") app.restore();
        else if (command == "reload") app.reload();
        else return false;
        return true;
    }

    // Встроенные команды и языковые вызовы проходят один и тот же путь.
    // Built-in commands and language calls follow the same path.
    void executeCommand(Application& app, const std::string& command, const std::string& target, const std::string& argument)
    {
        if (windowCommand(app, command)) return;
        if (command == "focus" && target.empty()) { app.focus(nullptr); return; }
        ui::Node& node = requireNode(app, target);
        if (command == "focus") app.focus(&node);
        else if (command == "remove") app.remove(node);
        else if (command == "showDialog") app.showDialog(node);
        else if (command == "hideDialog") app.hideDialog(node);
        else if (command == "showPopup") app.showPopup(node, requireNode(app, argument));
        else if (command == "hidePopup") app.hidePopup(node);
        else if (command == "navigate") app.navigate(node, argument);
        else if (command == "loadHtml") app.loadHtml(node, argument);
        else if (command == "postMessage") app.postWebMessage(node, argument);
        else if (command == "appendMarkup") app.appendMarkup(node, argument);
        else throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::CAPI_COMMAND, {command}));
    }
}
