#ifndef GRAN_UTILS_UTF8_HPP
#define GRAN_UTILS_UTF8_HPP

#include <cstddef>
#include <string>
#include <string_view>

namespace gran
{
    // Символ замены для испорченных последовательностей / The replacement character for broken sequences
    constexpr char32_t REPLACEMENT_CHARACTER = 0xFFFD;

    /*
    ** Следующий символ UTF-8 начиная с position; position сдвигается за него. Испорченная
    ** или избыточная последовательность и суррогаты дают U+FFFD и сдвиг на один байт —
    ** разбор никогда не зацикливается и не выходит за строку.
    **
    ** The next UTF-8 character starting at position; position moves past it. A broken
    ** or overlong sequence and surrogates give U+FFFD and a one byte move —
    ** decoding never loops forever and never goes beyond the string.
    */
    char32_t decodeUtf8(std::string_view text, std::size_t& position) noexcept;

    // Дописывает символ в UTF-8 (недопустимый — как U+FFFD) / Appends a character in UTF-8 (an invalid one as U+FFFD)
    void appendUtf8(std::string& text, char32_t symbol);

    // Число символов в строке / The character count of the string
    std::size_t countUtf8(std::string_view text) noexcept;
}

#endif
