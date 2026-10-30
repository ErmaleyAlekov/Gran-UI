#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Включённая команда с нажатым сочетанием среди своих / An enabled command with the pressed combination among its own
        bool matchesKey(const ui::Command& command, ui::Shortcut pressed) noexcept
        {
            if (!command.enabled)
                return false;
            for (const ui::Shortcut& key : command.keys)
            {
                if (ui::sameShortcut(key, pressed))
                    return true;
            }
            return false;
        }

        // Глубина узла (корень — 0) / The node depth (the root is 0)
        int nodeDepth(const ui::Node& node) noexcept
        {
            int depth = 0;

            for (const ui::Node* current = node.parent; current != nullptr; current = current->parent)
                ++depth;
            return depth;
        }

        /*
        ** Команда focus своего окна: within ищется по id в момент нажатия (узлы могли замениться), должен
        ** содержать фокус, а при модальном диалоге — лежать внутри него. Из нескольких — самый глубокий within.
        **
        ** A focus command of the own window: within is looked up by id at the press moment (the nodes might be replaced), it must
        ** hold the focus, and with a modal dialog — lie inside it. Of several — the deepest within.
        */
        ui::Command* focusCommand(const CommandSource& source, const CommandQuery& query)
        {
            ui::Command* best = nullptr;
            int bestDepth = -1;

            if (query.focus == nullptr)
                return nullptr;
            for (ui::Command& command : *source.commands)
            {
                if (command.scope != ui::CommandScope::FOCUS || !matchesKey(command, query.pressed))
                    continue;
                ui::Node* area = ui::findNode(*source.root, command.within);
                if (area == nullptr || !within(query.focus, *area))
                    continue;
                if (query.modal != nullptr && !within(area, *query.modal))
                    continue;
                if (nodeDepth(*area) > bestDepth)
                {
                    best = &command;
                    bestDepth = nodeDepth(*area);
                }
            }
            return best;
        }

        // Первая подходящая команда окна: своего — window и global, чужого — только global / The first matching window command: of the own one — window and global, of another one — only global
        ui::Command* windowCommand(const CommandSource& source, ui::Shortcut pressed)
        {
            for (ui::Command& command : *source.commands)
            {
                bool scoped = command.scope == ui::CommandScope::GLOBAL
                    || (source.own && command.scope == ui::CommandScope::WINDOW);
                if (scoped && matchesKey(command, pressed))
                    return &command;
            }
            return nullptr;
        }
    }

    // Модальный диалог блокирует команды окна под ним, как и ввод мышью / A modal dialog blocks the window commands under it, as the mouse input
    ui::Command* matchCommand(const std::vector<CommandSource>& sources, const CommandQuery& query)
    {
        ui::Command* found = nullptr;

        for (const CommandSource& source : sources)
        {
            if (found == nullptr && source.own && source.commands != nullptr && source.root != nullptr)
                found = focusCommand(source, query);
        }
        if (found != nullptr || query.modal != nullptr)
            return found;
        for (const CommandSource& source : sources)
        {
            if (source.commands != nullptr)
                found = windowCommand(source, query.pressed);
            if (found != nullptr)
                return found;
        }
        return nullptr;
    }
}
