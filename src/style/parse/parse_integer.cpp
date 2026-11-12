#include "gran/style/value_parsers.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cmath>
#include <string>

namespace gran::style
{
    /*
    ** Сравнение идёт в double до приведения (в double любой int точен, в float INT_MAX округлился бы вверх):
    ** к int приводится только число, уже лежащее в [minimum, maximum], поэтому приведение всегда определено.
    ** parseNumber уже отсёк ∞, NaN и выход за пределы float.
    **
    ** The comparison is done in double before the cast (any int is exact in double, in float INT_MAX would round up):
    ** only a number already lying in [minimum, maximum] is cast to int, so the cast is always defined.
    ** parseNumber has already cut off ∞, NaN and going beyond the float range.
    */
    int parseInteger(std::string_view text, int minimum, int maximum)
    {
        double value = parseNumber(text);

        if (value < static_cast<double>(minimum) || value > static_cast<double>(maximum) || std::floor(value) != value)
            throw GranException(ErrorCode::STYLE_VALUE, i18n::format(i18n::Message::STYLE_INTEGER_RANGE,
                {std::string(text), std::to_string(minimum), std::to_string(maximum)}));
        return static_cast<int>(value);
    }
}
