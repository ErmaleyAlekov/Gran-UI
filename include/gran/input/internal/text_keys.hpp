#ifndef GRAN_INPUT_INTERNAL_TEXT_KEYS_HPP
#define GRAN_INPUT_INTERNAL_TEXT_KEYS_HPP

#include "gran/platform/host.hpp"
#include "gran/platform/window.hpp"

#include <cstdint>

namespace gran::input::internal
{
    /*
    ** Смысл клавиши правки текста: куда шагнуть каретке или до куда стереть. Перевод «клавиша + модификаторы →
    ** смысл» делается по соглашениям ОС (text_keys.cpp) и только тогда, когда клавишу обрабатывает поле в фокусе.
    ** Платформа отдаёт клавиши как есть (Cmd = MODIFIER_CONTROL, Option = MODIFIER_ALT): переводи её реализация
    ** Option+← в Mod+←, такое нажатие вне поля запустило бы <Command key="Mod+Left"> (замечание Т-43).
    **
    ** The meaning of a text editing key: where the caret steps or up to where to erase. The translation "key + modifiers →
    ** meaning" follows the OS conventions (text_keys.cpp) and happens only when the focused field handles the key.
    ** The platform gives the keys as they are (Cmd = MODIFIER_CONTROL, Option = MODIFIER_ALT): if its implementation
    ** translated Option+← into Mod+←, such a press outside a field would run <Command key="Mod+Left"> (remark T-43).
    */
    enum class TextMove : std::uint8_t
    {
        NONE,
        CHAR_BACK,
        CHAR_FORWARD,
        // К началу слова слева (все ОС) / To the start of the word on the left (all OSes)
        WORD_BACK,
        // К концу слова справа (macOS, Linux) / To the end of the word on the right (macOS, Linux)
        WORD_END,
        // К началу следующего слова (Windows) / To the start of the next word (Windows)
        WORD_NEXT,
        // Края строки; у однострочного поля — края текста / The line edges; in a single-line field — the text edges
        LINE_START,
        LINE_END,
        TEXT_START,
        TEXT_END,
        // Начало и конец абзаца; уже на краю — к соседнему абзацу (macOS Option+↑/↓)
        // The paragraph start and end; already at the edge — to the neighbour paragraph (macOS Option+↑/↓)
        PARAGRAPH_BACK,
        PARAGRAPH_FORWARD,
        // Края абзаца без перехода к соседнему (macOS Control+A/E, Control+K стирает до конца)
        // The paragraph edges without moving to the neighbour (macOS Control+A/E, Control+K erases to the end)
        PARAGRAPH_START,
        PARAGRAPH_END,
        LINE_UP,
        LINE_DOWN,
        PAGE_UP,
        PAGE_DOWN
    };

    // ---- text_keys.cpp ----

    /*
    ** Шаг каретки по стрелкам, Home/End, PageUp/PageDown. Shift не учитывается (он только растягивает выделение).
    ** Windows и Linux: Ctrl+←/→ — слово, Ctrl+Home/End — края текста. macOS: Option+←/→ — слово, Option+↑/↓ — абзац,
    ** Cmd+←/→ — края строки, Cmd+↑/↓ — края текста, Home/End — края текста, Control+A/E — края абзаца (emacs).
    ** Прочие сочетания — NONE.
    **
    ** The caret step by the arrows, Home/End, PageUp/PageDown. Shift is ignored (it only stretches the selection).
    ** Windows and Linux: Ctrl+←/→ — a word, Ctrl+Home/End — the text edges. macOS: Option+←/→ — a word, Option+↑/↓ — a paragraph,
    ** Cmd+←/→ — the line edges, Cmd+↑/↓ — the text edges, Home/End — the text edges, Control+A/E — the paragraph edges (emacs).
    ** Other chords — NONE.
    */
    TextMove caretMove(platform::Key key, std::uint8_t modifiers, platform::HostOs os) noexcept;

    /*
    ** До куда стереть по Backspace (назад) и Delete (вперёд): символ; слово — с Ctrl (Windows, Linux) или Option
    ** (macOS); на macOS Cmd+Backspace / Cmd+Delete — до края строки, Control+K — до конца абзаца. Прочие — NONE.
    **
    ** Up to where Backspace (backward) and Delete (forward) erase: a character; a word — with Ctrl (Windows, Linux) or
    ** Option (macOS); on macOS Cmd+Backspace / Cmd+Delete — up to the line edge, Control+K — to the paragraph end. Others — NONE.
    */
    TextMove eraseMove(platform::Key key, std::uint8_t modifiers, platform::HostOs os) noexcept;

    // ---- text_keys_mac.cpp ----

    // Шаг каретки по соглашениям macOS; chord — модификаторы без Shift / The caret step by the macOS conventions; chord is the modifiers without Shift
    TextMove macCaretMove(platform::Key key, std::uint8_t chord) noexcept;

    // Стирание по соглашениям macOS; chord — модификаторы без Shift / Erasing by the macOS conventions; chord is the modifiers without Shift
    TextMove macEraseMove(platform::Key key, std::uint8_t chord) noexcept;
}

#endif
