#ifndef CHAT_COMMON_TEXT_UTILS_HPP
#define CHAT_COMMON_TEXT_UTILS_HPP

#include <string_view>

namespace chat
{
    // Значение шестнадцатеричной цифры 0–15; не цифра — -1 / The hex digit value 0–15; not a digit — -1
    int hexDigit(char symbol) noexcept;

    // Сравнение ASCII без учёта регистра (имена заголовков HTTP) / An ASCII case-insensitive comparison (HTTP header names)
    bool equalsIgnoreCase(std::string_view first, std::string_view second) noexcept;

    // Без пробелов, табуляций и переводов строк по краям / Without spaces, tabs and line breaks around
    std::string_view trimSpaces(std::string_view text) noexcept;
}

#endif
