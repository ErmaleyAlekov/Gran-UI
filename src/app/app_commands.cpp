#include "gran/app/internal/app_state.hpp"
#include "gran/core/i18n.hpp"

#include "gran/log/log.hpp"

namespace gran::app
{
    // Своё окно знает все свои команды; чужие окна делятся только глобальными / The own window knows all its commands; the other windows share only the global ones
    ui::Command* findCommand(Application::State& state, AppWindow& own, const std::string& id)
    {
        for (ui::Command& command : own.document.commands)
        {
            if (command.id == id)
                return &command;
        }
        for (const std::unique_ptr<AppWindow>& other : state.windows)
        {
            if (other.get() == &own)
                continue;
            for (ui::Command& command : other->document.commands)
            {
                if (command.id == id && command.scope == ui::CommandScope::GLOBAL)
                    return &command;
            }
        }
        return nullptr;
    }

    /*
    ** Имя действия копируется до вызова: обработчик может перезагрузить документ, и команда
    ** (она живёт в документе) исчезнет вместе с ним.
    **
    ** The action name is copied before the call: the handler may reload the document, and the command
    ** (it lives in the document) disappears together with it.
    */
    void runCommand(AppWindow& win, const ui::Command& command, ui::Node& node)
    {
        std::string name = command.action;

        if (name.empty())
            name = command.id;
        win.action(name, node);
    }

    // Источники поиска: это окно первым, затем остальные по порядку / The search sources: this window first, then the others in order
    bool AppWindow::shortcut(const input::CommandQuery& query)
    {
        std::vector<input::CommandSource> sources;
        ui::Node* node = query.focus;

        sources.push_back(input::CommandSource{&document.commands, document.root.get(), true});
        for (const std::unique_ptr<AppWindow>& other : shared.windows)
        {
            if (other.get() != this)
                sources.push_back(input::CommandSource{&other->document.commands, other->document.root.get(), false});
        }
        ui::Command* found = input::matchCommand(sources, query);
        if (found == nullptr)
            return false;
        if (node == nullptr)
            node = document.root.get();
        if (node != nullptr)
            runCommand(*this, *found, *node);
        return true;
    }

    void AppWindow::editingChanged()
    {
        refreshUndoCommands(shared, std::string());
    }

    // Ошибка ссылки не завершает программу: разметку можно писать раньше команд другого окна / A reference error does not end the program: the markup can be written before the commands of another window
    void AppWindow::command(const std::string& id, ui::Node& node)
    {
        ui::Command* found = findCommand(shared, *this, id);

        if (found == nullptr)
        {
            log::write(log::Level::WARNING, "command", i18n::format(i18n::Message::APP_UNKNOWN_COMMAND, {id}));
            return;
        }
        if (found->enabled)
            runCommand(*this, *found, node);
    }
}
