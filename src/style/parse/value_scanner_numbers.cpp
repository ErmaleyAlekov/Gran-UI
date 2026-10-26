#include "gran/style/value_scanner.hpp"

#include <cmath>

namespace gran::style
{
    // Цифры накапливаются в double; в дробной части растёт делитель
    // Digits accumulate in a double; in the fractional part the divisor grows
    std::size_t ValueScanner::readDigits(double& accumulator, double& scale, bool fraction) noexcept
    {
        std::size_t count = 0;

        while (_position < _text.size() && _text[_position] >= '0' && _text[_position] <= '9')
        {
            accumulator = accumulator * 10.0 + (_text[_position] - '0');
            if (fraction)
                scale *= 10.0;
            ++_position;
            ++count;
        }
        return count;
    }

    // Показатель читается, только если за 'e' идут знак и цифры (иначе это, например, единица "em")
    // The exponent is read only if 'e' is followed by a sign and digits (otherwise it is e.g. the "em" unit)
    double ValueScanner::readExponent() noexcept
    {
        double exponent = 0.0;
        double unused = 1.0;
        double sign = 1.0;
        std::size_t start = _position;

        if (_position >= _text.size() || (_text[_position] != 'e' && _text[_position] != 'E'))
            return 0.0;
        ++_position;
        if (_position < _text.size() && (_text[_position] == '-' || _text[_position] == '+'))
        {
            if (_text[_position] == '-')
                sign = -1.0;
            ++_position;
        }
        if (readDigits(exponent, unused, false) == 0)
        {
            _position = start;
            return 0.0;
        }
        return sign * exponent;
    }

    /*
    ** Число без локали. Хотя бы одна цифра обязательна ("." и "-" — не числа);
    ** при неудаче позиция возвращается назад.
    **
    ** A locale independent number. At least one digit is required ("." and "-" are not numbers);
    ** on failure the position is moved back.
    */
    bool ValueScanner::readNumber(float& value) noexcept
    {
        double mantissa = 0.0;
        double scale = 1.0;
        double sign = 1.0;

        atEnd();
        std::size_t start = _position;
        if (consume('-'))
            sign = -1.0;
        else
            consume('+');
        std::size_t digits = readDigits(mantissa, scale, false);
        if (_position < _text.size() && _text[_position] == '.')
        {
            ++_position;
            digits += readDigits(mantissa, scale, true);
        }
        if (digits == 0)
        {
            _position = start;
            return false;
        }
        value = static_cast<float>(sign * mantissa / scale * std::pow(10.0, readExponent()));
        return true;
    }
}
