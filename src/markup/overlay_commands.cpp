#include "gran/markup/internal/overlay.hpp"

#include <sstream>

namespace gran::markup::internal
{
    namespace
    {
        // Сравниваем разобранные клавиши, чтобы учитывать равнозначные имена модификаторов.
        // Compare parsed shortcuts to account for equivalent modifier names.
        bool overlaps(const beresta::Element& command, const std::vector<ui::Shortcut>& keys)
        {
            std::vector<ui::Shortcut> existing;
            ui::parseShortcuts(command.getAttribute("key").value_or(""), existing);
            for (const auto& old : existing)
                for (const auto& key : keys)
                    if (ui::sameShortcut(old, key))
                        return true;
            return false;
        }
    }

    // Явное разрешение автора позволяет заменить поведение кнопки командой палитры.
    // Explicit author permission allows a palette command to replace the button action.
    void applyRebind(OverlayContext& context, const beresta::Element& op)
    {
        auto* target = overlayTarget(context, op, "id");
        if (!target) return;
        requirePermission(*target, Customization::COMMAND);
        const auto& command = *findAttribute(op, "command");
        if (target->getLocalName() != "Button" || target->hasAttribute("windowRole")
            || !overlayResource(context, "Palette", "CommandRef", "ref", command))
            denyOverlay(op, command);
        target->removeAttribute("onClick");
        target->removeAttribute("popup");
        target->setAttribute("command", command);
    }

    // Проверяем конфликт с другими командами окна до публикации профиля.
    // Check for a conflict with other window commands before publishing the profile.
    void applyShortcut(OverlayContext& context, const beresta::Element& op)
    {
        const auto& id = *findAttribute(op, "command");
        const auto& text = *findAttribute(op, "key");
        std::vector<ui::Shortcut> keys;
        if (!overlayResource(context, "Palette", "CommandRef", "ref", id) || !ui::parseShortcuts(text, keys))
            denyOverlay(op, id);
        auto* resources = firstElementChild(*context.candidate.dom->getRoot());
        for (auto* cmd = firstElementChild(*resources); cmd; cmd = nextElementSibling(*cmd))
        {
            if (cmd->getLocalName() != "Command")
                continue;
            if (cmd->getAttribute("id") == id)
                const_cast<beresta::Element*>(cmd)->setAttribute("key", text);
            else if (overlaps(*cmd, keys))
                denyOverlay(op, text);
        }
    }

    // Имя команды учитывает уже добавленных потомков шаблона и ресурсы приложения.
    // Include cloned descendants and application resources when naming the generated command.
    static std::string sequenceId(const OverlayContext& context, const std::string& owner)
    {
        std::set<std::string> ids;
        collectMarkupIds(*context.candidate.dom->getRoot(), ids);
        const auto base = owner + ".command";
        auto id = base;
        for (std::size_t index = 1; ids.count(id); ++index) id = base + std::to_string(index);
        return id;
    }

    // Составная команда ссылается только на команды авторской палитры.
    // A composite command can reference only commands in the author's palette.
    std::string addSequence(const OverlayContext& context, const beresta::Element& op, const std::string& label)
    {
        const auto& sequence = *findAttribute(op, "sequence");
        std::istringstream words(sequence);
        std::string id;
        if (!(words >> id))
            denyOverlay(op, sequence);
        do
        {
            if (!overlayResource(context, "Palette", "CommandRef", "ref", id))
                denyOverlay(op, id);
        } while (words >> id);
        id = sequenceId(context, *findAttribute(op, "id"));
        auto command = std::make_unique<beresta::Element>("Command");
        command->setAttribute("id", id);
        command->setAttribute("text", label);
        command->setAttribute("sequence", sequence);
        auto* resources = firstElementChild(*context.candidate.dom->getRoot());
        for (auto* child = firstElementChild(*resources); child; child = nextElementSibling(*child))
            if (child->getLocalName() == "Palette")
                const_cast<beresta::Element*>(resources)->insertChild(child->getIndexInParent(), std::move(command));
        return id;
    }
}
