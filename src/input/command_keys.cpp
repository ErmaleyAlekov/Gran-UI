#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        constexpr std::uint8_t CHORD = platform::MODIFIER_CONTROL | platform::MODIFIER_ALT;

        // Ctrl+клавиша, которую поле правит само: буфер обмена, выделение, отмена, слова и края текста, отправка
        // A Ctrl+key the field edits by itself: the clipboard, the selection, undo, words and text edges, submitting
        bool editingChord(platform::Key key) noexcept
        {
            switch (key)
            {
                case platform::Key::A: case platform::Key::C: case platform::Key::X: case platform::Key::V:
                case platform::Key::Z: case platform::Key::Y: case platform::Key::INSERT: case platform::Key::BACKSPACE:
                case platform::Key::DELETE_KEY: case platform::Key::LEFT: case platform::Key::RIGHT: case platform::Key::UP:
                case platform::Key::DOWN: case platform::Key::HOME: case platform::Key::END: case platform::Key::ENTER:
                    return true;
                default:
                    return false;
            }
        }

        /*
        ** Поле ввода забирает набор и правку: всё без Ctrl и Alt (кроме F1–F12), Ctrl+Alt (AltGr набирает
        ** символы: ą, €) и Ctrl-сочетания правки. Alt+… и прочие Ctrl+… остаются командам.
        **
        ** An input takes typing and editing: everything without Ctrl and Alt (except F1–F12), Ctrl+Alt (AltGr types
        ** characters: ą, €) and the Ctrl editing chords. Alt+… and the other Ctrl+… stay with the commands.
        */
        bool fieldOwns(const platform::Event& event) noexcept
        {
            std::uint8_t chord = static_cast<std::uint8_t>(event.modifiers & CHORD);
            bool function = event.key >= platform::Key::F1 && event.key <= platform::Key::F12;

            if (chord == 0)
                return !function;
            if (chord == CHORD)
                return true;
            if (chord == platform::MODIFIER_CONTROL)
                return editingChord(event.key);
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
                return fieldOwns(event);
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

        if (event.key == platform::Key::UNKNOWN || (layer != nullptr && layer != modal) || focusOwns(state, event))
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
