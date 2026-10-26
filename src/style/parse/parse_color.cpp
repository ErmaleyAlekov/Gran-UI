#include "gran/style/value_parsers.hpp"
#include "gran/style/internal/named_colors.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_scanner.hpp"

#include <algorithm>
#include <string>

namespace gran::style
{
    using internal::equalsIgnoreCase;
    using internal::throwValueError;

    namespace
    {
        // Шестнадцатеричная цифра или -1 / A hexadecimal digit or -1
        int hexDigit(char symbol) noexcept
        {
            if (symbol >= '0' && symbol <= '9')
                return symbol - '0';
            if (symbol >= 'a' && symbol <= 'f')
                return symbol - 'a' + 10;
            if (symbol >= 'A' && symbol <= 'F')
                return symbol - 'A' + 10;
            return -1;
        }

        /*
        ** #rgb, #rgba, #rrggbb, #rrggbbaa. В коротких формах цифра удваивается (#f80 = #ff8800).
        **
        ** #rgb, #rgba, #rrggbb, #rrggbbaa. In the short forms a digit is doubled (#f80 = #ff8800).
        */
        Color parseHex(std::string_view text, std::string_view original)
        {
            std::size_t size = text.size();
            int bytes[4] = {0, 0, 0, 255};
            std::size_t width = 2;

            if (size == 3 || size == 4)
                width = 1;
            else if (size != 6 && size != 8)
                throwValueError(i18n::Message::STYLE_BAD_COLOR, original);
            for (std::size_t channel = 0; channel * width < size; ++channel)
            {
                int high = hexDigit(text[channel * width]);
                int low = hexDigit(text[channel * width + width - 1]);
                if (high < 0 || low < 0)
                    throwValueError(i18n::Message::STYLE_BAD_COLOR, original);
                bytes[channel] = high * 16 + low;
            }
            return colorFromBytes(static_cast<std::uint8_t>(bytes[0]), static_cast<std::uint8_t>(bytes[1]),
                static_cast<std::uint8_t>(bytes[2]), static_cast<std::uint8_t>(bytes[3]));
        }

        /*
        ** Аргументы rgb()/hsl(): "a, b, c[, d]" или "a b c [/ d]" (CSS Color 4).
        ** Результат — 3 или 4 строки; иначе ошибка.
        **
        ** The rgb()/hsl() arguments: "a, b, c[, d]" or "a b c [/ d]" (CSS Color 4).
        ** The result is 3 or 4 strings; otherwise an error.
        */
        std::vector<std::string_view> colorArguments(std::string_view inside, std::string_view original)
        {
            std::vector<std::string_view> parts = splitTopLevel(inside, ',');

            if (parts.size() == 1)
            {
                std::vector<std::string_view> slash = splitTopLevel(inside, '/');
                parts = splitSpaces(slash[0]);
                if (slash.size() == 2)
                    parts.push_back(slash[1]);
            }
            if (parts.size() != 3 && parts.size() != 4)
                throwValueError(i18n::Message::STYLE_BAD_COLOR, original);
            return parts;
        }

        // Канал rgb: 0..255 или проценты; альфа: 0..1 или проценты
        // An rgb channel: 0..255 or percent; the alpha: 0..1 or percent
        float colorChannel(std::string_view text, float scale)
        {
            if (!text.empty() && text.back() == '%')
                return std::min(1.0f, std::max(0.0f, parseFraction(text)));
            return std::min(1.0f, std::max(0.0f, parseNumber(text) / scale));
        }

        // Функции rgb(), rgba(), hsl(), hsla() / The rgb(), rgba(), hsl(), hsla() functions
        Color parseColorFunction(std::string_view name, std::string_view inside, std::string_view original)
        {
            std::vector<std::string_view> parts = colorArguments(inside, original);
            float alpha = 1.0f;

            if (parts.size() == 4)
                alpha = colorChannel(parts[3], 1.0f);
            if (equalsIgnoreCase(name, "rgb") || equalsIgnoreCase(name, "rgba"))
                return Color{colorChannel(parts[0], 255.0f), colorChannel(parts[1], 255.0f),
                    colorChannel(parts[2], 255.0f), alpha};
            if (!equalsIgnoreCase(name, "hsl") && !equalsIgnoreCase(name, "hsla"))
                throwValueError(i18n::Message::STYLE_BAD_COLOR, original);
            return colorFromHsl(parseAngle(parts[0]) * 57.2957795f, parseFraction(parts[1]),
                parseFraction(parts[2]), alpha);
        }
    }

    // Порядок проверки: #hex, функция, имя / The check order: #hex, a function, a name
    Color parseColor(std::string_view text)
    {
        std::string_view trimmed = trimSpaces(text);
        ValueScanner scanner(trimmed);
        std::vector<std::string_view> arguments;
        Color color;

        if (!trimmed.empty() && trimmed[0] == '#')
            return parseHex(trimmed.substr(1), text);
        std::string_view name = scanner.readIdentifier();
        if (!name.empty() && scanner.peek() == '(')
        {
            std::size_t open = trimmed.find('(');
            if (!scanner.readArguments(arguments) || !scanner.atEnd())
                throwValueError(i18n::Message::STYLE_BAD_COLOR, text);
            return parseColorFunction(name, trimmed.substr(open + 1, trimmed.size() - open - 2), text);
        }
        if (name.size() != trimmed.size() || !internal::findNamedColor(name, color))
            throwValueError(i18n::Message::STYLE_BAD_COLOR, text);
        return color;
    }
}
