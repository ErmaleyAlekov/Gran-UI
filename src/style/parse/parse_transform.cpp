#include "gran/style/value_parsers.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_scanner.hpp"

namespace gran::style
{
    using internal::equalsIgnoreCase;
    using internal::throwValueError;

    namespace
    {
        // Описание функции преобразования: имя, вид, ось (0 — обе, 1 — X, 2 — Y), угловая ли
        // A transform function description: the name, the kind, the axis (0 — both, 1 — X, 2 — Y), whether angular
        struct TransformFunction
        {
            const char* name;
            TransformKind kind;
            int axis;
            bool angular;
        };

        // Все поддерживаемые функции / All supported functions
        const TransformFunction FUNCTIONS[] = {
            {"translate", TransformKind::TRANSLATE, 0, false}, {"translateX", TransformKind::TRANSLATE, 1, false},
            {"translateY", TransformKind::TRANSLATE, 2, false}, {"scale", TransformKind::SCALE, 0, false},
            {"scaleX", TransformKind::SCALE, 1, false}, {"scaleY", TransformKind::SCALE, 2, false},
            {"rotate", TransformKind::ROTATE, 1, true}, {"skew", TransformKind::SKEW, 0, true},
            {"skewX", TransformKind::SKEW, 1, true}, {"skewY", TransformKind::SKEW, 2, true}};

        // Значение аргумента: угол или число / An argument value: an angle or a number
        float argumentValue(const TransformFunction& function, std::string_view argument)
        {
            if (function.angular)
                return parseAngle(argument);
            return parseNumber(argument);
        }

        /*
        ** Шаг из функции. Для одной оси вторая компонента — нейтральная (1 у scale, 0 у прочих);
        ** у translate(x) и skew(x) Y = 0, у scale(s) Y = s, как в CSS.
        **
        ** A step from a function. For one axis the other component is neutral (1 for scale, 0 otherwise);
        ** for translate(x) and skew(x) Y = 0, for scale(s) Y = s, as in CSS.
        */
        TransformStep makeStep(const TransformFunction& function, const std::vector<std::string_view>& arguments,
            std::string_view text)
        {
            float neutral = 0.0f;
            TransformStep step{function.kind, 0.0f, 0.0f};

            if (function.kind == TransformKind::SCALE)
                neutral = 1.0f;
            if (arguments.empty() || arguments.size() > 2 || (function.axis != 0 && arguments.size() != 1))
                throwValueError(i18n::Message::STYLE_BAD_TRANSFORM, text);
            float first = argumentValue(function, arguments[0]);
            float second = neutral;
            if (arguments.size() == 2)
                second = argumentValue(function, arguments[1]);
            else if (function.kind == TransformKind::SCALE)
                second = first;
            step.x = first;
            step.y = second;
            if (function.axis == 2)
                step = TransformStep{function.kind, neutral, first};
            if (function.axis == 1 && function.kind != TransformKind::ROTATE)
                step.y = neutral;
            return step;
        }
    }

    // Функции подряд через пробелы: "translate(0, -2) scale(1.02)"
    // Functions in a row separated by spaces: "translate(0, -2) scale(1.02)"
    Transform parseTransform(std::string_view text)
    {
        ValueScanner scanner(text);
        Transform transform;

        if (equalsIgnoreCase(trimSpaces(text), "none"))
            return transform;
        while (!scanner.atEnd())
        {
            std::string_view name = scanner.readIdentifier();
            std::vector<std::string_view> arguments;
            const TransformFunction* found = nullptr;
            for (const TransformFunction& function : FUNCTIONS)
            {
                if (name == function.name)
                    found = &function;
            }
            if (found == nullptr || !scanner.readArguments(arguments))
                throwValueError(i18n::Message::STYLE_BAD_TRANSFORM, text);
            transform.push_back(makeStep(*found, arguments, text));
        }
        return transform;
    }
}
