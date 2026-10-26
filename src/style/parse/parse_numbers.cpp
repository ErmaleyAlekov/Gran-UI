#include "gran/style/value_parsers.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_scanner.hpp"

namespace gran::style
{
    using internal::equalsIgnoreCase;
    using internal::splitNumberUnit;
    using internal::throwValueError;

    // Только число, без единиц / Only a number, without units
    float parseNumber(std::string_view text)
    {
        float value = 0.0f;
        std::string_view unit;

        if (!splitNumberUnit(trimSpaces(text), value, unit) || !unit.empty())
            throwValueError(i18n::Message::STYLE_BAD_NUMBER, text);
        return value;
    }

    // Без единицы и с "px" — пиксели, с "%" — проценты, "auto" — авто
    // Without a unit and with "px" — pixels, with "%" — percent, "auto" — auto
    Length parseLength(std::string_view text)
    {
        std::string_view trimmed = trimSpaces(text);
        Length length;
        std::string_view unit;

        if (equalsIgnoreCase(trimmed, "auto"))
            return length;
        if (!splitNumberUnit(trimmed, length.value, unit))
            throwValueError(i18n::Message::STYLE_BAD_LENGTH, text);
        if (unit.empty() || equalsIgnoreCase(unit, "px"))
            length.unit = LengthUnit::PIXELS;
        else if (unit == "%")
            length.unit = LengthUnit::PERCENT;
        else
            throwValueError(i18n::Message::STYLE_BAD_LENGTH, text);
        return length;
    }

    // "ms" и "s"; отрицательная длительность недопустима
    // "ms" and "s"; a negative duration is not allowed
    float parseDuration(std::string_view text)
    {
        float value = 0.0f;
        std::string_view unit;

        if (!splitNumberUnit(trimSpaces(text), value, unit) || value < 0.0f)
            throwValueError(i18n::Message::STYLE_BAD_DURATION, text);
        if (equalsIgnoreCase(unit, "ms"))
            return value / 1000.0f;
        if (!equalsIgnoreCase(unit, "s"))
            throwValueError(i18n::Message::STYLE_BAD_DURATION, text);
        return value;
    }

    // deg (и число без единицы), rad, turn, grad → радианы
    // deg (and a bare number), rad, turn, grad → radians
    float parseAngle(std::string_view text)
    {
        const float pi = 3.14159265358979f;
        float value = 0.0f;
        std::string_view unit;

        if (!splitNumberUnit(trimSpaces(text), value, unit))
            throwValueError(i18n::Message::STYLE_BAD_NUMBER, text);
        if (unit.empty() || equalsIgnoreCase(unit, "deg"))
            return value * pi / 180.0f;
        if (equalsIgnoreCase(unit, "rad"))
            return value;
        if (equalsIgnoreCase(unit, "turn"))
            return value * 2.0f * pi;
        if (equalsIgnoreCase(unit, "grad"))
            return value * pi / 200.0f;
        throwValueError(i18n::Message::STYLE_BAD_NUMBER, text);
    }

    // "50%" → 0.5, "0.5" → 0.5 / "50%" → 0.5, "0.5" → 0.5
    float parseFraction(std::string_view text)
    {
        float value = 0.0f;
        std::string_view unit;

        if (!splitNumberUnit(trimSpaces(text), value, unit))
            throwValueError(i18n::Message::STYLE_BAD_NUMBER, text);
        if (unit == "%")
            return value / 100.0f;
        if (!unit.empty())
            throwValueError(i18n::Message::STYLE_BAD_NUMBER, text);
        return value;
    }
}
