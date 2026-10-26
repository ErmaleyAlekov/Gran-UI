#include "gran/style/value_parsers.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_scanner.hpp"

namespace gran::style
{
    using internal::equalsIgnoreCase;
    using internal::throwValueError;

    namespace
    {
        // Лексема начинается как число / The token starts as a number
        bool startsNumber(std::string_view token) noexcept
        {
            if (token.empty())
                return false;
            char first = token[0];
            return (first >= '0' && first <= '9') || first == '-' || first == '+' || first == '.';
        }

        // Части тени, собранные из лексем / The shadow parts collected from the tokens
        struct ShadowParts
        {
            float numbers[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            std::size_t count = 0;
            bool hasColor = false;
        };

        // Одна лексема тени: "inset", число или цвет / One shadow token: "inset", a number or a color
        void readShadowToken(std::string_view token, std::string_view text, ShadowParts& parts, Shadow& shadow)
        {
            if (equalsIgnoreCase(token, "inset"))
                shadow.inset = true;
            else if (startsNumber(token) && parts.count < 4)
                parts.numbers[parts.count++] = parseNumber(token);
            else if (!parts.hasColor && !startsNumber(token))
            {
                shadow.color = parseColor(token);
                parts.hasColor = true;
            }
            else
                throwValueError(i18n::Message::STYLE_BAD_SHADOW, text);
        }

        /*
        ** Одна тень: 2–4 числа подряд (x, y, размытие, растяжение), цвет и "inset"
        ** в любом месте, как в CSS. Размытие не может быть отрицательным.
        **
        ** One shadow: 2–4 numbers in a row (x, y, blur, spread), a color and "inset"
        ** anywhere, as in CSS. The blur cannot be negative.
        */
        Shadow parseOneShadow(std::string_view text)
        {
            ShadowParts parts;
            Shadow shadow;

            for (std::string_view token : splitSpaces(text))
                readShadowToken(token, text, parts, shadow);
            if (parts.count < 2 || !parts.hasColor || parts.numbers[2] < 0.0f)
                throwValueError(i18n::Message::STYLE_BAD_SHADOW, text);
            shadow.offsetX = parts.numbers[0];
            shadow.offsetY = parts.numbers[1];
            shadow.blur = parts.numbers[2];
            shadow.spread = parts.numbers[3];
            return shadow;
        }
    }

    // "none", цвет или функция-градиент / "none", a color or a gradient function
    Paint parsePaint(std::string_view text)
    {
        std::string_view trimmed = trimSpaces(text);
        Paint paint;

        if (equalsIgnoreCase(trimmed, "none"))
            return paint;
        if (trimmed.find("gradient(") != std::string_view::npos)
        {
            paint.kind = PaintKind::GRADIENT;
            paint.gradient = parseGradient(trimmed);
            return paint;
        }
        paint.kind = PaintKind::COLOR;
        paint.color = parseColor(trimmed);
        return paint;
    }

    // Тени через запятую верхнего уровня (запятые внутри rgb() не делят)
    // Shadows separated by top level commas (commas inside rgb() do not split)
    std::vector<Shadow> parseShadows(std::string_view text)
    {
        std::vector<Shadow> shadows;

        if (equalsIgnoreCase(trimSpaces(text), "none"))
            return shadows;
        for (std::string_view part : splitTopLevel(text, ','))
            shadows.push_back(parseOneShadow(part));
        if (shadows.empty())
            throwValueError(i18n::Message::STYLE_BAD_SHADOW, text);
        return shadows;
    }
}
