#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    namespace
    {
        // Своя команда или глобальная другого окна; неизвестная узел не меняет / The own command or a global one of another window; an unknown one does not change the node
        void bindNode(AppWindow& win, ui::Node& node)
        {
            const ui::Command* command = findCommand(win.shared, win, node.content.command);

            if (command != nullptr)
                ui::applyCommand(node, *command);
        }

        // Обход с шаблонами списков: копии записей берутся из шаблона уже привязанными; only — только эта команда
        // A walk with the list templates: the record copies are taken from a template already bound; only — this command only
        void bindTree(AppWindow& win, ui::Node& node, const std::string* only)
        {
            if (!node.content.command.empty() && (only == nullptr || *only == node.content.command))
                bindNode(win, node);
            for (const std::unique_ptr<ui::Node>& child : node.children)
                bindTree(win, *child, only);
            if (node.itemTemplate)
                bindTree(win, *node.itemTemplate, only);
        }
    }

    /*
    ** Значение кода важнее разметки: после перезагрузки выключенная командой кнопка остаётся выключенной.
    ** Команды отмены и повтора важнее и того, и другого: их включённость — состояние стека.
    **
    ** The code value beats the markup: after a reload a button disabled by its command stays disabled.
    ** The undo and redo commands beat both: their enabled value is the stack state.
    */
    void applyCommandStates(AppWindow& win)
    {
        for (ui::Command& command : win.document.commands)
        {
            auto state = win.shared.commandStates.find(command.id);
            if (state != win.shared.commandStates.end())
                command.enabled = state->second;
        }
        linkUndoCommands(win.shared, win);
    }

    // Все команды поддерева: при загрузке документа и вставке узлов / All the subtree commands: on loading a document and inserting nodes
    void bindCommands(AppWindow& win, ui::Node& subtree)
    {
        bindTree(win, subtree, nullptr);
    }

    // Глобальная команда одного окна управляет кнопками других, поэтому обходятся все окна / A global command of one window drives the buttons of the others, so all the windows are walked
    void refreshCommand(Application::State& state, const std::string& id)
    {
        for (const std::unique_ptr<AppWindow>& win : state.windows)
        {
            if (win->document.root)
                bindTree(*win, *win->document.root, &id);
        }
    }
}
