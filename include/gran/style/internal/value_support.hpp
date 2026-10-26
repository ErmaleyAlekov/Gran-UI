#ifndef GRAN_STYLE_INTERNAL_VALUE_SUPPORT_HPP
#define GRAN_STYLE_INTERNAL_VALUE_SUPPORT_HPP

#include "gran/core/message.hpp"

#include <string_view>

namespace gran::style::internal
{
    // Бросает GranException(STYLE_VALUE) с сообщением id и аргументами text, extra
    // Throws GranException(STYLE_VALUE) with the message id and the arguments text, extra
    [[noreturn]] void throwValueError(i18n::Message id, std::string_view text, std::string_view extra = {});

    /*
    ** Делит "12.5px" на число и единицу ("px", "%", "" ...). false — в начале нет числа
    ** или после единицы есть что-то ещё.
    **
    ** Splits "12.5px" into a number and a unit ("px", "%", "" ...). false — there is no number
    ** at the start or there is something after the unit.
    */
    bool splitNumberUnit(std::string_view text, float& value, std::string_view& unit) noexcept;

    // Сравнение ASCII без учёта регистра / An ASCII case-insensitive comparison
    bool equalsIgnoreCase(std::string_view first, std::string_view second) noexcept;
}

#endif
