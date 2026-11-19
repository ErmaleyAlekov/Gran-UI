#include "gran/input/input_router.hpp"
#include "gran/input/internal/text_history.hpp"
#include "gran/utils/utf8.hpp"

namespace gran::input
{
    namespace
    {
        // Символ с Mod (без AltGr) или с META (Win/Super, физический Control на macOS) — не набор / A character with Mod (without AltGr) or with META (Win/Super, the physical Control on macOS) is not typing
        bool commandCharacter(const platform::Event& event) noexcept
        {
            bool control = (event.modifiers & platform::MODIFIER_CONTROL) != 0;
            bool alt = (event.modifiers & platform::MODIFIER_ALT) != 0;

            return (control && !alt) || (event.modifiers & platform::MODIFIER_META) != 0;
        }
    }

    /*
    ** Управляющие символы (Enter, Tab, Backspace приходят и символом) и пробел у списка не начинают правку: пробел
    ** отмечает строку, остальные обработала клавиша.
    **
    ** Control characters (Enter, Tab, Backspace come as a character too) and space at a list do not start an edit: space
    ** marks the row, the others were handled by the key.
    */
    bool typeCharacter(RouterState& state, const platform::Event& event)
    {
        ui::Node* focus = state.focus;
        std::string text;

        if (focus == nullptr || commandCharacter(event))
            return false;
        appendUtf8(text, event.character);
        if (!ui::isTextField(*focus))
        {
            if (focus->content.selection.empty() || event.character <= 0x20 || event.character == 0x7F)
                return false;
            return state.actions->listText(*focus, text);
        }
        if (focus->content.readOnly)
            return false;
        return internal::typeText(state, *focus, text);
    }
}
