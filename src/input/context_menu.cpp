#include "gran/input/internal/text_cursor.hpp"
#include "gran/core/i18n.hpp"

#include <memory>

namespace gran::input
{
    namespace
    {
        // Встроенные команды; номер команды хранится в content.value пункта / The built-in commands; the command number is kept in the item content.value
        enum class Command : int { COPY, PASTE, SELECT_ALL };

        // Пункт меню с текстом из каталога сообщений (язык интерфейса) / A menu item with the text from the message catalog (the interface language)
        void addItem(ui::Node& menu, i18n::Message text, Command command)
        {
            std::unique_ptr<ui::Node> item = std::make_unique<ui::Node>();

            item->kind = ui::ElementKind::MENU_ITEM;
            item->content.text = i18n::format(text);
            item->content.value = static_cast<float>(command);
            ui::appendChild(menu, std::move(item));
        }

        // Доступность пунктов по цели: копировать — есть выделение (не пароль), вставить — поле не только для чтения
        // The item availability by the target: copy — there is a selection (not a password), paste — a field that is not read-only
        void prepareItems(RouterState& state, const ui::Node& target)
        {
            std::size_t from = 0;
            std::size_t to = 0;
            bool selected = internal::selectionRange(target.content, from, to) && !target.content.password;
            const std::vector<std::unique_ptr<ui::Node>>& items = state.contextMenu->children;

            ui::setEnabled(*items[0], selected);
            ui::setEnabled(*items[1], ui::isTextField(target) && !target.content.readOnly);
            ui::setEnabled(*items[2], !target.content.text.empty());
        }
    }

    // Меню создаётся один раз и только скрывается и показывается / The menu is created once and is only hidden and shown
    ui::Node* createContextMenu(ui::Node& root)
    {
        std::unique_ptr<ui::Node> menu = std::make_unique<ui::Node>();

        menu->kind = ui::ElementKind::POPUP;
        menu->visible = false;
        addItem(*menu, i18n::Message::MENU_COPY, Command::COPY);
        addItem(*menu, i18n::Message::MENU_PASTE, Command::PASTE);
        addItem(*menu, i18n::Message::MENU_SELECT_ALL, Command::SELECT_ALL);
        ui::Node& added = ui::appendChild(root, std::move(menu));
        ui::markDirty(added, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        return &added;
    }

    /*
    ** Правый щелчок, как в браузере: внутри выделения оно сохраняется, вне — каретка встаёт
    ** в точку щелчка; поле получает фокус. Меню открывается в точке мыши.
    **
    ** A right click, as in a browser: inside the selection it is kept, outside — the caret stands
    ** at the click point; a field gets the focus. The menu opens at the mouse point.
    */
    void contextClick(RouterState& state, const platform::Event& event)
    {
        ui::Node* hit = pickNode(state, event.position);
        ui::Node* control = controlOf(hit);
        std::size_t from = 0;
        std::size_t to = 0;

        if (control != nullptr && disabled(control))
            control = nullptr;
        ui::Node* target = textTarget(hit, control);
        if (target == nullptr || state.contextMenu == nullptr || state.text == nullptr)
            return;
        std::size_t offset = state.text->offsetAt(*target, event.position);
        bool inside = state.selection == target && internal::selectionRange(target->content, from, to)
            && offset >= from && offset <= to;
        platform::Event press = event;
        press.clicks = 1;
        press.modifiers = 0;
        if (!inside)
            beginTextSelection(state, *target, press);
        state.selecting = nullptr;
        if (ui::isTextField(*target))
            changeFocus(state, target);
        prepareItems(state, *target);
        state.contextTarget = target;
        openLayer(state, *state.contextMenu, Rect{event.position.x, event.position.y, 0.0f, 0.0f}, false);
    }

    // Команда выполняется над целью меню; вставка — только в поле / The command runs on the menu target; pasting — only into a field
    bool runContextCommand(RouterState& state, const ui::Node& item)
    {
        Command command = static_cast<Command>(static_cast<int>(item.content.value));

        if (state.contextMenu == nullptr || item.parent != state.contextMenu || state.contextTarget == nullptr)
            return false;
        ui::Node& target = *state.contextTarget;
        if (command == Command::COPY)
            internal::copySelection(state, target);
        else if (command == Command::PASTE && ui::isTextField(target) && state.window != nullptr)
            insertText(state, target, state.window->clipboardText());
        else if (command == Command::SELECT_ALL)
            selectAll(state, target);
        return true;
    }
}
