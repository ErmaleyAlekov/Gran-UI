#ifndef GRAN_UI_COMMAND_HPP
#define GRAN_UI_COMMAND_HPP

#include "gran/platform/window.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gran::ui
{
    struct Node;

    /*
    ** Сочетание клавиш: клавиша и биты модификаторов (platform::MODIFIER_*). В разметке — "Mod+S",
    ** "Ctrl+Shift+Z", "F5"; Mod — Ctrl (на macOS станет Cmd).
    **
    ** A key combination: the key and the modifier bits (platform::MODIFIER_*). In the markup it is "Mod+S",
    ** "Ctrl+Shift+Z", "F5"; Mod is Ctrl (on macOS it will become Cmd).
    */
    struct Shortcut
    {
        platform::Key key = platform::Key::UNKNOWN;
        std::uint8_t modifiers = 0;
    };

    /*
    ** Где действует сочетание команды: WINDOW — в своём окне, FOCUS — пока фокус внутри элемента within,
    ** GLOBAL — во всех окнах приложения (но не во всей системе).
    **
    ** Where the command shortcut works: WINDOW — in its own window, FOCUS — while the focus is inside the within element,
    ** GLOBAL — in all the application windows (but not system-wide).
    */
    enum class CommandScope : std::uint8_t { WINDOW, FOCUS, GLOBAL };

    /*
    ** Команда <Command id text action key scope within enabled> (паттерн Command, как QAction): одно
    ** действие для кнопок, пунктов меню и горячих клавиш. Пустой action — действие с именем id.
    ** enabled общий для всех привязанных элементов: выключенная команда выключает их.
    **
    ** A <Command id text action key scope within enabled> command (the Command pattern, as QAction): one
    ** action for buttons, menu items and shortcuts. An empty action is the action named id.
    ** enabled is shared by all the bound controls: a disabled command disables them.
    */
    struct Command
    {
        std::string id;
        std::string text;
        std::string action;
        std::vector<Shortcut> keys;
        CommandScope scope = CommandScope::WINDOW;
        std::string within;
        bool enabled = true;
    };

    // ---- shortcut_parse.cpp ----

    // Одно сочетание "Mod+Shift+S" (регистр не важен); false — неверная запись / One "Mod+Shift+S" combination (the case does not matter); false — a wrong notation
    bool parseShortcut(std::string_view text, Shortcut& result) noexcept;

    // Сочетания через пробел ("Mod+Y Mod+Shift+Z"); false — неверное хотя бы одно / Combinations separated by spaces ("Mod+Y Mod+Shift+Z"); false — at least one is wrong
    bool parseShortcuts(std::string_view list, std::vector<Shortcut>& result);

    // ---- shortcut_label.cpp ----

    // Подпись для меню и доступности: "Ctrl+Shift+S", "Del", "F5" / The label for menus and accessibility: "Ctrl+Shift+S", "Del", "F5"
    std::string shortcutText(Shortcut shortcut);

    // Та же клавиша с теми же модификаторами / The same key with the same modifiers
    bool sameShortcut(Shortcut left, Shortcut right) noexcept;

    // ---- command_bind.cpp ----

    /*
    ** Узел с command="id" получает от команды enabled (состояние Disabled), её подпись — только если своей
    ** нет и нет детей (кнопка-иконка подписи не получит), и подпись первого сочетания (меню, доступность).
    **
    ** A node with command="id" gets from the command enabled (the Disabled state), its label — only if there is no own one
    ** and no children (an icon button does not get a label), and the first combination label (menus, accessibility).
    */
    void applyCommand(Node& node, const Command& command);

    // Привязать узлы поддерева и шаблонов списков к командам своего документа (неизвестные id — пропуск)
    // Bind the subtree and list template nodes to the own document commands (unknown ids are skipped)
    void bindCommandTree(Node& subtree, const std::vector<Command>& commands);
}

#endif
