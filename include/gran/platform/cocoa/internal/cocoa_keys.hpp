#ifndef GRAN_PLATFORM_COCOA_INTERNAL_KEYS_HPP
#define GRAN_PLATFORM_COCOA_INTERNAL_KEYS_HPP

#include "gran/platform/window.hpp"

namespace gran::platform::cocoa
{
    /*
    ** Клавиша по физическому коду macOS (NSEvent.keyCode, константы kVK_* из Carbon HIToolbox): код задаёт место
    ** клавиши, а не символ, поэтому Cmd+C работает в русской раскладке. Неизвестный код — Key::UNKNOWN.
    ** Чистый C++ (cocoa_keys.cpp): таблицу можно проверить и без AppKit.
    **
    ** The key by the macOS physical code (NSEvent.keyCode, the kVK_* constants from Carbon HIToolbox): the code sets
    ** the key place, not the symbol, so Cmd+C works in the Russian layout. An unknown code is Key::UNKNOWN.
    ** Plain C++ (cocoa_keys.cpp): the table can be checked without AppKit too.
    */
    Key keyFromCode(unsigned short code) noexcept;
}

#endif
