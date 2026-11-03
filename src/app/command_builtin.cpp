#include "gran/app/internal/app_state.hpp"
#include "gran/app/commands.hpp"
#include "gran/core/gran_exception.hpp"

#include "gran/log/log.hpp"

namespace gran::app
{
    namespace
    {
        // Части действия gran:команда[:id[:аргумент]] / The parts of a gran:command[:id[:argument]] action
        struct BuiltinCall
        {
            std::string command;
            std::string target;
            std::string argument;
        };

        // Разбор после префикса gran: (аргумент может сам содержать двоеточия) / Parsing after the gran: prefix (the argument may contain colons itself)
        BuiltinCall parseBuiltin(const std::string& action)
        {
            BuiltinCall call;
            std::size_t first = action.find(':', 5);
            std::size_t second = std::string::npos;

            call.command = action.substr(5, first - 5);
            if (first == std::string::npos)
                return call;
            second = action.find(':', first + 1);
            call.target = action.substr(first + 1, second - first - 1);
            if (second != std::string::npos)
                call.argument = action.substr(second + 1);
            return call;
        }

        // Команды окна без id относятся к окну самого узла: gran:close в дополнительном окне закрывает его
        // Window commands without an id belong to the node own window: gran:close in an extra window closes it
        bool windowBuiltin(AppWindow& win, const std::string& command)
        {
            platform::Window* window = win.window.get();

            if (command != "close" && command != "minimize" && command != "maximize" && command != "restore")
                return false;
            if (window == nullptr)
                return true;
            if (command == "close")
                window->close();
            else if (command == "minimize")
                window->minimize();
            else if (command == "maximize")
                window->maximize();
            else
                window->restore();
            return true;
        }

        /*
        ** gran:undo и gran:redo без имени стека сначала работают с редактируемым полем окна (фокус или поле,
        ** откуда открыли меню), как Ctrl+Z; нечего отменять в поле — основной стек приложения.
        **
        ** gran:undo and gran:redo without a stack name work with the edited field of the window first (the focus or the field
        ** the menu was opened from), as Ctrl+Z does; nothing to undo in the field — the main application stack.
        */
        bool fieldHistory(AppWindow& win, const std::string& command)
        {
            if ((command != "undo" && command != "redo") || !win.router)
                return false;
            return win.router->undoField(command == "redo");
        }

        // Якорь меню без аргумента — сам узел (ему не нужен id); захват указателя без id — тоже сам узел
        // A menu anchor without an argument is the node itself (it needs no id); the pointer capture without an id is the node itself too
        void runBuiltin(Application::State& state, const BuiltinCall& call, ui::Node& node)
        {
            if (call.target.empty() && windowBuiltin(windowOf(state, node), call.command))
                return;
            // gran:close:результат — закрыть окно узла с результатом для onClosed (без onClose, как closeWindow)
            // gran:close:result — close the node window with a result for onClosed (without onClose, as closeWindow)
            if (call.command == "close")
            {
                std::string result = call.target;
                if (!call.argument.empty())
                    result += ":" + call.argument;
                markClosing(state, node, result);
                return;
            }
            if (call.target.empty() && fieldHistory(windowOf(state, node), call.command))
                return;
            if (call.target.empty() && treeRowBuiltin(state, call.command, node))
                return;
            if (call.target.empty() && call.command == "capturePointer")
                state.owner->capturePointer(node);
            else if (call.target.empty() && call.command == "releasePointer")
                state.owner->releasePointer(node);
            else if (call.command == "showPopup" && call.argument.empty())
                state.owner->showPopup(requireNode(*state.owner, call.target), node);
            else
                executeCommand(*state.owner, call.command, call.target, call.argument);
        }
    }

    /*
    ** gran:команда[:id[:аргумент]] зарезервировано ядром; прочие имена — бизнес-логике. Ошибка разметки
    ** (неизвестный id, команда) не завершает работающее приложение: текст из каталога уходит в stderr.
    **
    ** gran:command[:id[:argument]] is reserved by the core; other names belong to business logic. A markup error
    ** (an unknown id, a command) does not terminate the running application: the catalog text goes to stderr.
    */
    bool executeBuiltin(Application::State& state, const std::string& action, ui::Node& node)
    {
        if (action.compare(0, 5, "gran:") != 0 || state.owner == nullptr)
            return false;
        try
        {
            runBuiltin(state, parseBuiltin(action), node);
        }
        catch (const GranException& error)
        {
            // Сбои GPU и ОС остаются фатальными; ошибки разметки и аргументов — нет (в т. ч. gran:reload с неверным XML)
            // GPU and OS failures stay fatal; markup and argument errors do not (including gran:reload with invalid XML)
            if (error.getCode() == ErrorCode::GPU || error.getCode() == ErrorCode::DEVICE_LOST
                || error.getCode() == ErrorCode::PLATFORM)
                throw;
            log::write(log::Level::SEVERE, "markup", error.what());
        }
        return true;
    }
}
