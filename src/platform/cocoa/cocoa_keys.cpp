#include "gran/platform/cocoa/internal/cocoa_keys.hpp"

namespace gran::platform::cocoa
{
    namespace
    {
        // Пара «физический код macOS — клавиша движка» / A "macOS physical code — engine key" pair
        struct KeyCode
        {
            unsigned short code;
            Key key;
        };

        /*
        ** Коды kVK_* (Carbon HIToolbox/Events.h): буквы и цифры раскладки ANSI идут не по алфавиту — так они
        ** расположены на клавиатуре. Help (0x72) стоит на месте Insert у клавиатур Apple с цифровым блоком,
        ** Enter цифрового блока (0x4C) — тот же ENTER.
        **
        ** The kVK_* codes (Carbon HIToolbox/Events.h): the ANSI layout letters and digits are not in alphabetical
        ** order — that is how they lie on the keyboard. Help (0x72) sits at the Insert place on Apple keyboards with
        ** a numeric pad, the numeric pad Enter (0x4C) is the same ENTER.
        */
        constexpr KeyCode CODES[] = {
            {0x00, Key::A}, {0x01, Key::S}, {0x02, Key::D}, {0x03, Key::F}, {0x04, Key::H}, {0x05, Key::G},
            {0x06, Key::Z}, {0x07, Key::X}, {0x08, Key::C}, {0x09, Key::V}, {0x0B, Key::B}, {0x0C, Key::Q},
            {0x0D, Key::W}, {0x0E, Key::E}, {0x0F, Key::R}, {0x10, Key::Y}, {0x11, Key::T}, {0x1F, Key::O},
            {0x20, Key::U}, {0x22, Key::I}, {0x23, Key::P}, {0x25, Key::L}, {0x26, Key::J}, {0x28, Key::K},
            {0x2D, Key::N}, {0x2E, Key::M},
            {0x12, Key::DIGIT_1}, {0x13, Key::DIGIT_2}, {0x14, Key::DIGIT_3}, {0x15, Key::DIGIT_4},
            {0x17, Key::DIGIT_5}, {0x16, Key::DIGIT_6}, {0x1A, Key::DIGIT_7}, {0x1C, Key::DIGIT_8},
            {0x19, Key::DIGIT_9}, {0x1D, Key::DIGIT_0},
            {0x24, Key::ENTER}, {0x4C, Key::ENTER}, {0x30, Key::TAB}, {0x31, Key::SPACE}, {0x33, Key::BACKSPACE},
            {0x35, Key::ESCAPE}, {0x75, Key::DELETE_KEY}, {0x72, Key::INSERT}, {0x6E, Key::CONTEXT_MENU},
            {0x73, Key::HOME}, {0x77, Key::END}, {0x74, Key::PAGE_UP}, {0x79, Key::PAGE_DOWN},
            {0x7B, Key::LEFT}, {0x7C, Key::RIGHT}, {0x7D, Key::DOWN}, {0x7E, Key::UP},
            {0x7A, Key::F1}, {0x78, Key::F2}, {0x63, Key::F3}, {0x76, Key::F4}, {0x60, Key::F5}, {0x61, Key::F6},
            {0x62, Key::F7}, {0x64, Key::F8}, {0x65, Key::F9}, {0x6D, Key::F10}, {0x67, Key::F11}, {0x6F, Key::F12}
        };
    }

    // Поиск по таблице: она мала (65 пар), а нажатия редки / A table search: it is small (65 pairs), and presses are rare
    Key keyFromCode(unsigned short code) noexcept
    {
        for (const KeyCode& entry : CODES)
        {
            if (entry.code == code)
                return entry.key;
        }
        return Key::UNKNOWN;
    }
}
