#ifndef GRAN_DATA_INTERNAL_CELL_FORMATS_HPP
#define GRAN_DATA_INTERNAL_CELL_FORMATS_HPP

#include <string>
#include <string_view>

namespace gran::data::internal
{
    // ---- cell_number.cpp ----

    // Число по правилам normalizeValue(NUMBER) (края уже обрезаны); false — не число / A number by the normalizeValue(NUMBER) rules (the edges are already trimmed); false — not a number
    bool normalizeNumber(std::string_view text, std::string& result);

    // ---- cell_date.cpp ----

    // Дата ГГГГ-ММ-ДД или ДД.ММ.ГГГГ → ГГГГ-ММ-ДД (края уже обрезаны); false — не дата / A date YYYY-MM-DD or DD.MM.YYYY → YYYY-MM-DD (the edges are already trimmed); false — not a date
    bool normalizeDate(std::string_view text, std::string& result);
}

#endif
