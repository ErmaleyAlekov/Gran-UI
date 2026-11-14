#include "gran/app/app_driver.hpp"
#include "gran/utils/utf8.hpp"

namespace gran
{
    // KEY_DOWN и KEY_UP с одними модификаторами / KEY_DOWN and KEY_UP with the same modifiers
    void AppDriver::press(platform::Key key, std::uint8_t modifiers, std::size_t index)
    {
        platform::HeadlessWindow& target = window(index);
        platform::Event event;

        event.kind = platform::EventKind::KEY_DOWN;
        event.key = key;
        event.modifiers = modifiers;
        target.pushEvent(event);
        settle();
        event.kind = platform::EventKind::KEY_UP;
        target.pushEvent(event);
        settle();
    }

    /*
    ** Символы — как их отдаёт ОС после раскладки и IME: по одному CHARACTER на кодовую точку, без KEY_DOWN
    ** (буква «я» не связана с физической клавишей). Неверный UTF-8 даёт U+FFFD, как у decodeUtf8.
    **
    ** The characters — as the OS gives them after the layout and IME: one CHARACTER per code point, without KEY_DOWN
    ** (the letter "я" is not tied to a physical key). Invalid UTF-8 gives U+FFFD, as decodeUtf8 does.
    */
    void AppDriver::type(const std::string& text, std::size_t index)
    {
        platform::HeadlessWindow& target = window(index);
        std::size_t position = 0;
        platform::Event event;

        event.kind = platform::EventKind::CHARACTER;
        while (position < text.size())
        {
            event.character = decodeUtf8(text, position);
            target.pushEvent(event);
        }
        settle();
    }
}
