#include "gran/ui/command.hpp"
#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        // Команда документа по id или nullptr / A document command by id or nullptr
        const Command* commandById(const std::vector<Command>& commands, const std::string& id) noexcept
        {
            for (const Command& command : commands)
            {
                if (command.id == id)
                    return &command;
            }
            return nullptr;
        }
    }

    // Новая подпись меняет размер узла: нужна компоновка / A new label changes the node size: a layout is needed
    void applyCommand(Node& node, const Command& command)
    {
        std::string shortcut;

        setEnabled(node, command.enabled);
        if (!command.keys.empty())
            shortcut = shortcutText(command.keys.front());
        if (node.content.text.empty() && !command.text.empty() && node.children.empty())
        {
            node.content.text = command.text;
            markDirty(node, DIRTY_LAYOUT | DIRTY_PAINT);
        }
        if (node.content.shortcut != shortcut)
        {
            node.content.shortcut = shortcut;
            markDirty(node, DIRTY_LAYOUT | DIRTY_PAINT);
        }
    }

    // Шаблоны списков — тоже: копии записей берутся из шаблона уже привязанными / The list templates too: the record copies are taken from a template already bound
    void bindCommandTree(Node& subtree, const std::vector<Command>& commands)
    {
        const Command* command = nullptr;

        if (!subtree.content.command.empty())
            command = commandById(commands, subtree.content.command);
        if (command != nullptr)
            applyCommand(subtree, *command);
        for (const std::unique_ptr<Node>& child : subtree.children)
            bindCommandTree(*child, commands);
        if (subtree.itemTemplate)
            bindCommandTree(*subtree.itemTemplate, commands);
    }
}
