#include "gran/input/internal/text_keys.hpp"

namespace gran::input::internal
{
    namespace
    {
        // Сочетания emacs в полях Cocoa (физический Control = MODIFIER_META): A и E — края абзаца
        // The emacs chords in Cocoa fields (the physical Control = MODIFIER_META): A and E are the paragraph edges
        TextMove emacsMove(platform::Key key) noexcept
        {
            if (key == platform::Key::A)
                return TextMove::PARAGRAPH_START;
            if (key == platform::Key::E)
                return TextMove::PARAGRAPH_END;
            return TextMove::NONE;
        }

        // Option — слово (←/→) и абзац (↑/↓), Cmd — края строки (←/→) и текста (↑/↓)
        // Option — a word (←/→) and a paragraph (↑/↓), Cmd — the line edges (←/→) and the text edges (↑/↓)
        TextMove chordMove(platform::Key key, std::uint8_t chord) noexcept
        {
            bool option = chord == platform::MODIFIER_ALT;
            bool command = chord == platform::MODIFIER_CONTROL;

            if (option && key == platform::Key::LEFT)
                return TextMove::WORD_BACK;
            if (option && key == platform::Key::RIGHT)
                return TextMove::WORD_END;
            if (option && key == platform::Key::UP)
                return TextMove::PARAGRAPH_BACK;
            if (option && key == platform::Key::DOWN)
                return TextMove::PARAGRAPH_FORWARD;
            if (command && key == platform::Key::LEFT)
                return TextMove::LINE_START;
            if (command && key == platform::Key::RIGHT)
                return TextMove::LINE_END;
            if (command && key == platform::Key::UP)
                return TextMove::TEXT_START;
            if (command && key == platform::Key::DOWN)
                return TextMove::TEXT_END;
            return TextMove::NONE;
        }

        // Без модификаторов; Home/End на macOS — края текста / Without modifiers; Home/End on macOS are the text edges
        TextMove plainMove(platform::Key key) noexcept
        {
            switch (key)
            {
                case platform::Key::LEFT: return TextMove::CHAR_BACK;
                case platform::Key::RIGHT: return TextMove::CHAR_FORWARD;
                case platform::Key::UP: return TextMove::LINE_UP;
                case platform::Key::DOWN: return TextMove::LINE_DOWN;
                case platform::Key::HOME: return TextMove::TEXT_START;
                case platform::Key::END: return TextMove::TEXT_END;
                case platform::Key::PAGE_UP: return TextMove::PAGE_UP;
                case platform::Key::PAGE_DOWN: return TextMove::PAGE_DOWN;
                default: return TextMove::NONE;
            }
        }
    }

    // chord — модификаторы без Shift / chord is the modifiers without Shift
    TextMove macCaretMove(platform::Key key, std::uint8_t chord) noexcept
    {
        if (chord == platform::MODIFIER_META)
            return emacsMove(key);
        if (chord != 0)
            return chordMove(key, chord);
        return plainMove(key);
    }

    // Option — слово, Cmd — до края строки, Control+K — до конца абзаца (emacs) / Option — a word, Cmd — to the line edge, Control+K — to the paragraph end (emacs)
    TextMove macEraseMove(platform::Key key, std::uint8_t chord) noexcept
    {
        bool back = key == platform::Key::BACKSPACE;

        if (chord == platform::MODIFIER_META && key == platform::Key::K)
            return TextMove::PARAGRAPH_END;
        if (!back && key != platform::Key::DELETE_KEY)
            return TextMove::NONE;
        if (chord == 0 && back)
            return TextMove::CHAR_BACK;
        if (chord == 0)
            return TextMove::CHAR_FORWARD;
        if (chord == platform::MODIFIER_ALT && back)
            return TextMove::WORD_BACK;
        if (chord == platform::MODIFIER_ALT)
            return TextMove::WORD_END;
        if (chord == platform::MODIFIER_CONTROL && back)
            return TextMove::LINE_START;
        if (chord == platform::MODIFIER_CONTROL)
            return TextMove::LINE_END;
        return TextMove::NONE;
    }
}
