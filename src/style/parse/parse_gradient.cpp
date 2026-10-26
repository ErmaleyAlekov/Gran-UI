#include "gran/style/value_parsers.hpp"
#include "gran/style/internal/gradient_support.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_scanner.hpp"

namespace gran::style
{
    using internal::equalsIgnoreCase;
    using internal::throwValueError;

    namespace
    {
        // Сторона "to …" → угол CSS в градусах (углы — приближение 45° кратными)
        // A "to …" side → a CSS angle in degrees (corners are approximated by 45° multiples)
        float sideAngle(std::string_view sides, std::string_view text)
        {
            static const Keyword SIDES[] = {
                {"top", 0}, {"right", 90}, {"bottom", 180}, {"left", 270},
                {"top right", 45}, {"right top", 45}, {"bottom right", 135}, {"right bottom", 135},
                {"bottom left", 225}, {"left bottom", 225}, {"top left", 315}, {"left top", 315}};
            std::string normalized;

            for (std::string_view part : splitSpaces(sides))
            {
                if (!normalized.empty())
                    normalized += ' ';
                normalized.append(part.data(), part.size());
            }
            if (normalized.empty())
                throwValueError(i18n::Message::STYLE_BAD_GRADIENT, text, sides);
            return parseKeyword(normalized, SIDES, 12) * 3.14159265f / 180.0f;
        }

        // Заголовок linear: угол или "to сторона"; true — первый аргумент был заголовком
        // The linear header: an angle or "to side"; true — the first argument was a header
        bool linearHeader(std::string_view first, Gradient& gradient, std::string_view text)
        {
            ValueScanner scanner(first);
            std::string_view word = scanner.readIdentifier();

            if (equalsIgnoreCase(word, "to"))
            {
                gradient.angle = sideAngle(scanner.rest(), text);
                return true;
            }
            float unused = 0.0f;
            ValueScanner probe(first);
            if (!probe.readNumber(unused))
                return false;
            gradient.angle = parseAngle(first);
            return true;
        }

        /*
        ** Заголовок radial/conic: "circle", "ellipse", "from угол", "at x y" в любом сочетании.
        ** true — первый аргумент был заголовком (а не опорной точкой).
        **
        ** The radial/conic header: "circle", "ellipse", "from angle", "at x y" in any combination.
        ** true — the first argument was a header (not a stop).
        */
        bool shapeHeader(std::string_view first, Gradient& gradient, std::string_view text)
        {
            std::size_t at = first.find("at ");
            std::string_view shape = trimSpaces(first.substr(0, at));
            ValueScanner scanner(shape);
            std::string_view word = scanner.readIdentifier();

            if (at != 0 && word.empty())
                return false;
            if (equalsIgnoreCase(word, "circle"))
                gradient.circle = true;
            else if (equalsIgnoreCase(word, "from") && gradient.kind == GradientKind::CONIC)
                gradient.angle = parseAngle(scanner.rest());
            else if (!word.empty() && !equalsIgnoreCase(word, "ellipse"))
                return false;
            if (at != std::string_view::npos)
                gradient.center = internal::parseGradientCenter(first.substr(at + 3), text);
            return true;
        }
    }

    // Функция по имени, затем заголовок (если есть) и опорные точки
    // The function by name, then the header (if any) and the stops
    Gradient parseGradient(std::string_view text)
    {
        static const Keyword KINDS[] = {{"linear-gradient", static_cast<int>(GradientKind::LINEAR)},
            {"radial-gradient", static_cast<int>(GradientKind::RADIAL)},
            {"conic-gradient", static_cast<int>(GradientKind::CONIC)}};
        ValueScanner scanner(trimSpaces(text));
        std::string_view name = scanner.readIdentifier();
        std::vector<std::string_view> arguments;
        Gradient gradient;
        bool header = false;

        gradient.kind = static_cast<GradientKind>(parseKeyword(name, KINDS, 3));
        if (!scanner.readArguments(arguments) || !scanner.atEnd() || arguments.empty())
            throwValueError(i18n::Message::STYLE_BAD_GRADIENT, text, name);
        if (gradient.kind == GradientKind::CONIC)
            gradient.angle = 0.0f;
        if (gradient.kind == GradientKind::LINEAR)
            header = linearHeader(arguments[0], gradient, text);
        else
            header = shapeHeader(arguments[0], gradient, text);
        std::size_t firstStop = 0;
        if (header)
            firstStop = 1;
        gradient.stops = internal::parseStops(arguments, firstStop, text);
        return gradient;
    }
}
