#include "gran/ui/command.hpp"
#include "gran/ui/node.hpp"
#include "gran/ui/internal/preorder.hpp"

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

    /*
    ** Новая подпись меняет размер узла: нужна компоновка. Подпись команды может нести букву доступа («_Сохранить»):
    ** знак «_» убирается у любого узла, а буква работает у пунктов меню.
    **
    ** A new label changes the node size: a layout is needed. The command label may carry an access letter ("_Save"):
    ** the "_" sign is removed for any node, and the letter works for the menu items.
    */
    void applyCommand(Node& node, const Command& command)
    {
        std::string shortcut;

        setEnabled(node, command.enabled);
        if (!command.keys.empty())
            shortcut = shortcutText(command.keys.front());
        if (node.children.empty() && (node.content.text.empty() || node.content.text == node.content.inheritedCaption))
        {
            const auto previous = node.content.text;
            const auto previousMnemonic = node.content.mnemonic;
            const auto previousOffset = node.content.mnemonicAt;
            node.content.text = command.text;
            takeMnemonic(node);
            node.content.inheritedCaption = node.content.text;
            if (node.content.text != previous || node.content.mnemonic != previousMnemonic ||
                node.content.mnemonicAt != previousOffset)
                markDirty(node, DIRTY_LAYOUT | DIRTY_PAINT);
        }
        else
            node.content.inheritedCaption.clear();
        if (node.content.shortcut != shortcut)
        {
            node.content.shortcut = shortcut;
            markDirty(node, DIRTY_LAYOUT | DIRTY_PAINT);
        }
    }

    // Шаблоны списков — тоже: копии записей берутся из шаблона уже привязанными / The list templates too: the record copies are taken from a template already bound
    void bindCommandTree(Node& subtree, const std::vector<Command>& commands)
    {
        internal::Preorder<Node> walk(subtree, true);
        while (Node* node = walk.next())
        {
            if (node->content.command.empty())
                continue;
            if (const Command* command = commandById(commands, node->content.command))
                applyCommand(*node, *command);
        }
    }
}
