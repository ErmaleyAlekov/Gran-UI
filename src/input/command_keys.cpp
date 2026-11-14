#include "gran/input/input_router.hpp"
#include "gran/input/internal/text_keys.hpp"

namespace gran::input
{
    namespace
    {
        // Модификаторы, меняющие смысл клавиши; META входит (замечание Т-46: без него Control+… на macOS считался вводом)
        // The modifiers that change the key meaning; META is included (remark T-46: without it Control+… on macOS counted as typing)
        constexpr std::uint8_t CHORD = platform::MODIFIER_CONTROL | platform::MODIFIER_ALT | platform::MODIFIER_META;

        // Ctrl+Alt — это AltGr в Windows и Linux (набор символов) / Ctrl+Alt is AltGr on Windows and Linux (typing characters)
        constexpr std::uint8_t ALT_GR = platform::MODIFIER_CONTROL | platform::MODIFIER_ALT;

        /*
        ** Mod+клавиша, которую поле правит само: буфер обмена, выделение, отмена (шаги и стирание — caretMove/eraseMove).
        ** Mod+Y — повтор только в Windows и Linux (на macOS Cmd+Y — команда программы, повтор — Cmd+Shift+Z);
        ** Mod+Enter — только у TextArea (отправка), у однострочного поля он остаётся командам.
        **
        ** A Mod+key the field edits by itself: the clipboard, the selection, undo (the steps and erasing — caretMove/eraseMove).
        ** Mod+Y is redo only on Windows and Linux (on macOS Cmd+Y is a program command, redo is Cmd+Shift+Z);
        ** Mod+Enter only in a TextArea (submitting), in a single-line field it stays with the commands.
        */
        bool editingChord(const ui::Node& field, platform::Key key, platform::HostOs os) noexcept
        {
            switch (key)
            {
                case platform::Key::A: case platform::Key::C: case platform::Key::X: case platform::Key::V:
                case platform::Key::Z: case platform::Key::INSERT:
                    return true;
                case platform::Key::Y:
                    return os != platform::HostOs::MACOS;
                case platform::Key::ENTER:
                    return field.kind == ui::ElementKind::TEXT_AREA;
                default:
                    return false;
            }
        }

        /*
        ** Поле ввода забирает набор и правку: всё без Ctrl и Alt (кроме F1–F12), шаги каретки и стирание по таблице
        ** ОС (на macOS — и Option+←, Cmd+←: только здесь, в поле), Ctrl+Alt (AltGr набирает символы: ą, €; на macOS
        ** AltGr нет, Cmd+Option — команды) и Ctrl-сочетания правки. Alt+… и прочие Ctrl+… остаются командам.
        ** META (физический Control на macOS) — только по таблице (emacs Control+A/E/K), остальное — не поля.
        **
        ** An input takes typing and editing: everything without Ctrl and Alt (except F1–F12), the caret steps and erasing
        ** by the OS table (on macOS also Option+←, Cmd+←: only here, in a field), Ctrl+Alt (AltGr types characters: ą, €;
        ** macOS has no AltGr, Cmd+Option are commands) and the Ctrl editing chords. Alt+… and the other Ctrl+… stay with the commands.
        ** META (the physical Control on macOS) — only by the table (emacs Control+A/E/K), the rest is not the field's.
        */
        bool fieldOwns(const ui::Node& field, const platform::Event& event) noexcept
        {
            std::uint8_t chord = static_cast<std::uint8_t>(event.modifiers & CHORD);
            bool function = event.key >= platform::Key::F1 && event.key <= platform::Key::F12;
            platform::HostOs os = platform::hostOs();

            if (internal::caretMove(event.key, event.modifiers, os) != internal::TextMove::NONE
                || internal::eraseMove(event.key, event.modifiers, os) != internal::TextMove::NONE)
                return true;
            if (chord == 0)
                return !function;
            if (chord == ALT_GR)
                return os != platform::HostOs::MACOS;
            if (chord == platform::MODIFIER_CONTROL)
                return editingChord(field, event.key, os);
            return false;
        }

        // Стрелки, пробел и Enter — навигация и нажатие элемента в фокусе / The arrows, space and Enter are the focused control navigation and press
        bool controlKey(platform::Key key) noexcept
        {
            return key == platform::Key::SPACE || key == platform::Key::ENTER || key == platform::Key::LEFT
                || key == platform::Key::RIGHT || key == platform::Key::UP || key == platform::Key::DOWN;
        }

        // Клавиша, которую обработает элемент в фокусе или выделенный текст (Tab — всегда обход фокуса)
        // A key the focused control or the selected text will handle (Tab is always the focus walk)
        bool focusOwns(const RouterState& state, const platform::Event& event) noexcept
        {
            std::uint8_t chord = static_cast<std::uint8_t>(event.modifiers & CHORD);

            if (chord == 0 && event.key == platform::Key::TAB)
                return true;
            if (state.focus != nullptr && ui::isTextField(*state.focus))
                return fieldOwns(*state.focus, event);
            if (state.focus == nullptr && state.selection != nullptr)
                return chord == platform::MODIFIER_CONTROL && (event.key == platform::Key::A || event.key == platform::Key::C);
            return state.focus != nullptr && chord == 0 && controlKey(event.key);
        }
    }

    // Символы клавиши, ставшей командой, в поле не попадают (как отменённый keyDown) / The characters of a key that became a command do not get into a field (as a cancelled keyDown)
    bool commandKey(RouterState& state, const platform::Event& event)
    {
        ui::Node* layer = topLayer(state);
        ui::Node* modal = modalLayer(state);
        CommandQuery query;

        // MODIFIER_META (Win/Super, на macOS — физический Control) в командах не бывает: без проверки он срезался бы
        // маской ниже, и Control+S на macOS совпал бы с командой «S» (контракт window.hpp: с META команды не совпадают)
        // MODIFIER_META (Win/Super, the physical Control on macOS) never takes part in commands: without the check the mask
        // below would cut it, and Control+S on macOS would match the command "S" (the window.hpp contract: no match with META)
        if (event.key == platform::Key::UNKNOWN || (event.modifiers & platform::MODIFIER_META) != 0
            || (layer != nullptr && layer != modal) || focusOwns(state, event))
            return false;
        query.pressed.key = event.key;
        query.pressed.modifiers = static_cast<std::uint8_t>(event.modifiers
            & (platform::MODIFIER_CONTROL | platform::MODIFIER_SHIFT | platform::MODIFIER_ALT));
        query.focus = state.focus;
        query.modal = modal;
        if (!state.actions->shortcut(query))
            return false;
        state.suppressCharacter = true;
        return true;
    }
}
