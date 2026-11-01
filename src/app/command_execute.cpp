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

    // Предел стека — неотрицательное целое без знаков и пробелов (0 — без предела).
    // The stack limit is a non-negative integer without signs and spaces (0 — no limit).
    static std::size_t limitValue(const std::string& value)
    {
        std::size_t limit = 0;

        if (value.empty() || value.size() > 9)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::CAPI_ARGUMENT));
        for (char digit : value)
        {
            if (digit < '0' || digit > '9')
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::CAPI_ARGUMENT));
            limit = limit * 10 + static_cast<std::size_t>(digit - '0');
        }
        return limit;
    }

    // Команды <Command>, стеки отмены и переменные адресуются своими именами, а не id узлов (пустое имя стека — основной).
    // The <Command> commands, the undo stacks and the variables are addressed by their own names, not node ids (an empty stack name is the main one).
    static bool namedCommand(Application& app, const std::string& command, const std::string& target, const std::string& argument)
    {
        if (command == "runCommand") app.runCommand(target);
        else if (command == "enableCommand" || command == "disableCommand") app.setCommandEnabled(target, command == "enableCommand");
        else if (command == "undo") app.undo(target);
        else if (command == "redo") app.redo(target);
        else if (command == "clearUndo") app.undoStack(target).clear();
        else if (command == "setUndoClean") app.undoStack(target).setClean();
        else if (command == "setUndoLimit") app.undoStack(target).setLimit(limitValue(argument));
        else if (command == "setVariable") app.setVariable(target, argument);
        else if (command == "toggleVariable") app.setVariableBoolean(target, app.variable(target).value_or("") != "true");
        else return false;
        return true;
    }

    // Встроенные команды и языковые вызовы проходят один и тот же путь.
    // Built-in commands and language calls follow the same path.
    void executeCommand(Application& app, const std::string& command, const std::string& target, const std::string& argument)
    {
        if (windowCommand(app, command) || namedCommand(app, command, target, argument)) return;
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
        else if (command == "capturePointer") app.capturePointer(node);
        else if (command == "releasePointer") app.releasePointer(node);
        else throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::CAPI_COMMAND, {command}));
    }
}
