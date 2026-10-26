#include "gran/style/value_parsers.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_scanner.hpp"

namespace gran::style
{
    using internal::equalsIgnoreCase;
    using internal::throwValueError;

    namespace
    {
        // Именованные кривые CSS / The named CSS curves
        bool namedEasing(std::string_view name, Easing& easing)
        {
            static const struct { const char* name; float x1, y1, x2, y2; } CURVES[] = {
                {"ease", 0.25f, 0.1f, 0.25f, 1.0f}, {"ease-in", 0.42f, 0.0f, 1.0f, 1.0f},
                {"ease-out", 0.0f, 0.0f, 0.58f, 1.0f}, {"ease-in-out", 0.42f, 0.0f, 0.58f, 1.0f}};

            if (equalsIgnoreCase(name, "linear") || equalsIgnoreCase(name, "step-start")
                || equalsIgnoreCase(name, "step-end"))
            {
                easing.kind = EasingKind::LINEAR;
                if (!equalsIgnoreCase(name, "linear"))
                    easing.kind = EasingKind::STEPS;
                easing.jumpStart = equalsIgnoreCase(name, "step-start");
                return true;
            }
            for (const auto& curve : CURVES)
            {
                if (equalsIgnoreCase(name, curve.name))
                {
                    easing = Easing{EasingKind::CUBIC_BEZIER, curve.x1, curve.y1, curve.x2, curve.y2, 1, false};
                    return true;
                }
            }
            return false;
        }

        // cubic-bezier(x1, y1, x2, y2) с x в 0..1; steps(n[, start|end|jump-start|jump-end])
        // cubic-bezier(x1, y1, x2, y2) with x in 0..1; steps(n[, start|end|jump-start|jump-end])
        Easing functionEasing(std::string_view name, const std::vector<std::string_view>& arguments,
            std::string_view text)
        {
            static const Keyword JUMPS[] = {{"start", 1}, {"jump-start", 1}, {"end", 0}, {"jump-end", 0}};
            Easing easing;

            if (equalsIgnoreCase(name, "cubic-bezier") && arguments.size() == 4)
            {
                easing = Easing{EasingKind::CUBIC_BEZIER, parseNumber(arguments[0]), parseNumber(arguments[1]),
                    parseNumber(arguments[2]), parseNumber(arguments[3]), 1, false};
                if (easing.x1 < 0.0f || easing.x1 > 1.0f || easing.x2 < 0.0f || easing.x2 > 1.0f)
                    throwValueError(i18n::Message::STYLE_BAD_EASING, text);
                return easing;
            }
            if (!equalsIgnoreCase(name, "steps") || arguments.empty() || arguments.size() > 2)
                throwValueError(i18n::Message::STYLE_BAD_EASING, text);
            easing.kind = EasingKind::STEPS;
            easing.steps = static_cast<int>(parseNumber(arguments[0]));
            if (easing.steps < 1)
                throwValueError(i18n::Message::STYLE_BAD_EASING, text);
            if (arguments.size() == 2)
                easing.jumpStart = parseKeyword(arguments[1], JUMPS, 4) == 1;
            return easing;
        }
    }

    // Имя кривой или функция / A curve name or a function
    Easing parseEasing(std::string_view text)
    {
        ValueScanner scanner(trimSpaces(text));
        std::string_view name = scanner.readIdentifier();
        std::vector<std::string_view> arguments;
        Easing easing;

        if (scanner.atEnd() && namedEasing(name, easing))
            return easing;
        if (name.empty() || !scanner.readArguments(arguments) || !scanner.atEnd())
            throwValueError(i18n::Message::STYLE_BAD_EASING, text);
        return functionEasing(name, arguments, text);
    }

    /*
    ** Переходы: "свойство длительность [кривая] [задержка]" через запятую. Первое время —
    ** длительность, второе — задержка; всё прочее — кривая.
    **
    ** Transitions: "property duration [curve] [delay]" separated by commas. The first time
    ** is the duration, the second is the delay; anything else is the curve.
    */
    std::vector<TransitionSpec> parseTransitions(std::string_view text)
    {
        std::vector<TransitionSpec> transitions;

        for (std::string_view part : splitTopLevel(text, ','))
        {
            std::vector<std::string_view> tokens = splitSpaces(part);
            TransitionSpec spec;
            int times = 0;
            if (tokens.size() < 2 || tokens.size() > 4)
                throwValueError(i18n::Message::STYLE_BAD_TRANSITION, part);
            spec.property = std::string(tokens[0]);
            for (std::size_t index = 1; index < tokens.size(); ++index)
            {
                bool isTime = tokens[index][0] >= '0' && tokens[index][0] <= '9';
                if (isTime && times == 0)
                    spec.duration = parseDuration(tokens[index]);
                else if (isTime && times == 1)
                    spec.delay = parseDuration(tokens[index]);
                else
                    spec.easing = parseEasing(tokens[index]);
                times += static_cast<int>(isTime);
            }
            transitions.push_back(spec);
        }
        return transitions;
    }
}
