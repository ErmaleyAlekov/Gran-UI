#include "gran/input/internal/text_keys.hpp"

namespace gran::input::internal
{
    namespace
    {
        // Модификаторы, меняющие смысл клавиши (Shift — нет) / The modifiers that change the key meaning (Shift does not)
        constexpr std::uint8_t CHORD = platform::MODIFIER_CONTROL | platform::MODIFIER_ALT | platform::MODIFIER_META;

        // Шаг на слово вправо по соглашению ОС / A word step to the right by the OS convention
        TextMove wordForward(platform::HostOs os) noexcept
        {
            if (os == platform::HostOs::WINDOWS)
                return TextMove::WORD_NEXT;
            return TextMove::WORD_END;
        }

        // Windows и Linux: без модификаторов или с одним Ctrl / Windows and Linux: without modifiers or with Ctrl alone
        TextMove pcMove(platform::Key key, std::uint8_t chord, platform::HostOs os) noexcept
        {
            bool control = chord == platform::MODIFIER_CONTROL;

            if (chord != 0 && !control)
                return TextMove::NONE;
            if (key == platform::Key::LEFT && control)
                return TextMove::WORD_BACK;
            if (key == platform::Key::RIGHT && control)
                return wordForward(os);
            if (key == platform::Key::HOME && control)
                return TextMove::TEXT_START;
            if (key == platform::Key::END && control)
                return TextMove::TEXT_END;
            // Ctrl+PageUp/PageDown — переключение вкладок (команды), а не страница поля
            // Ctrl+PageUp/PageDown is tab switching (commands), not a field page
            if ((key == platform::Key::PAGE_UP || key == platform::Key::PAGE_DOWN) && control)
                return TextMove::NONE;
            switch (key)
            {
                case platform::Key::LEFT: return TextMove::CHAR_BACK;
                case platform::Key::RIGHT: return TextMove::CHAR_FORWARD;
                case platform::Key::HOME: return TextMove::LINE_START;
                case platform::Key::END: return TextMove::LINE_END;
                case platform::Key::UP: return TextMove::LINE_UP;
                case platform::Key::DOWN: return TextMove::LINE_DOWN;
                case platform::Key::PAGE_UP: return TextMove::PAGE_UP;
                case platform::Key::PAGE_DOWN: return TextMove::PAGE_DOWN;
                default: return TextMove::NONE;
            }
        }

        // Windows и Linux: символ без модификаторов, слово — с Ctrl / Windows and Linux: a character without modifiers, a word with Ctrl
        TextMove pcErase(platform::Key key, std::uint8_t chord, platform::HostOs os) noexcept
        {
            bool back = key == platform::Key::BACKSPACE;

            if (!back && key != platform::Key::DELETE_KEY)
                return TextMove::NONE;
            if (chord == 0 && back)
                return TextMove::CHAR_BACK;
            if (chord == 0)
                return TextMove::CHAR_FORWARD;
            if (chord == platform::MODIFIER_CONTROL && back)
                return TextMove::WORD_BACK;
            if (chord == platform::MODIFIER_CONTROL)
                return wordForward(os);
            return TextMove::NONE;
        }
    }

    // Соглашение выбирает ОС; Shift отбрасывается до сравнения / The OS picks the convention; Shift is dropped before the comparison
    TextMove caretMove(platform::Key key, std::uint8_t modifiers, platform::HostOs os) noexcept
    {
        std::uint8_t chord = static_cast<std::uint8_t>(modifiers & CHORD);

        if (os == platform::HostOs::MACOS)
            return macCaretMove(key, chord);
        return pcMove(key, chord, os);
    }

    // Направление — от клавиши, длина шага — от модификатора / The direction comes from the key, the step length from the modifier
    TextMove eraseMove(platform::Key key, std::uint8_t modifiers, platform::HostOs os) noexcept
    {
        std::uint8_t chord = static_cast<std::uint8_t>(modifiers & CHORD);

        if (os == platform::HostOs::MACOS)
            return macEraseMove(key, chord);
        return pcErase(key, chord, os);
    }
}
