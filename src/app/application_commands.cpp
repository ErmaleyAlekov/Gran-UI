#include "gran/app/internal/app_state.hpp"

namespace gran
{
    namespace
    {
        // Первая команда id по окнам (главное первым) и её окно; nullptr — нет такой / The first id command in the windows (the main one first) and its window; nullptr — none
        ui::Command* anyCommand(const Application::State& state, const std::string& id, app::AppWindow** owner)
        {
            for (const std::unique_ptr<app::AppWindow>& win : state.windows)
            {
                for (ui::Command& command : win->document.commands)
                {
                    if (command.id != id)
                        continue;
                    if (owner != nullptr)
                        *owner = win.get();
                    return &command;
                }
            }
            return nullptr;
        }
    }

    // Узел-исполнитель — корень окна команды: обработчику нужен узел, а нажатия не было / The running node is the command window root: the handler needs a node, and there was no press
    bool Application::runCommand(const std::string& id)
    {
        app::AppWindow* win = nullptr;
        ui::Command* command = anyCommand(*_state, id, &win);

        if (command == nullptr || !command->enabled || !win->document.root)
            return false;
        app::runCommand(*win, *command, *win->document.root);
        return true;
    }

    // Сначала значение для будущих документов, затем команды и узлы открытых окон / First the value for future documents, then the commands and the nodes of the open windows
    void Application::setCommandEnabled(const std::string& id, bool enabled)
    {
        _state->commandStates[id] = enabled;
        for (const std::unique_ptr<app::AppWindow>& win : _state->windows)
        {
            for (ui::Command& command : win->document.commands)
            {
                if (command.id == id)
                    command.enabled = enabled;
            }
        }
        app::refreshCommand(*_state, id);
    }

    // Неизвестная команда выключена: её нечем выполнить / An unknown command is disabled: there is nothing to run
    bool Application::commandEnabled(const std::string& id) const
    {
        const ui::Command* command = anyCommand(*_state, id, nullptr);

        return command != nullptr && command->enabled;
    }
}
