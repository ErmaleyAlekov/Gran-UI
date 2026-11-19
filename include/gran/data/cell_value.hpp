#ifndef GRAN_DATA_CELL_VALUE_HPP
#define GRAN_DATA_CELL_VALUE_HPP

#include "gran/data/list_source.hpp"

#include <string>
#include <string_view>

namespace gran::data
{
    // ---- cell_value.cpp ----

    /*
    ** Привести введённый текст к виду значения столбца типа type и проверить его. Пробелы по краям
    ** убираются; пустое значение подходит любому типу (поле не заполнено).
    ** - TEXT — любой текст как есть (края не трогаются: пробелы могут быть частью значения);
    ** - NUMBER — [+|-]цифры[(.|,)цифры][e[+|-]цифры]; запятая становится точкой, «+» убирается;
    ** - DATE — ГГГГ-ММ-ДД или ДД.ММ.ГГГГ (день и месяц — одна или две цифры) → ГГГГ-ММ-ДД, дата существует;
    ** - BOOLEAN — true/false, 1/0, yes/no, on/off, да/нет (латиница без учёта регистра) → «true» / «false».
    ** false — текст не подходит типу (text не меняется).
    **
    ** Bring the entered text to the value form of a column of type type and check it. The spaces at the edges
    ** are removed; an empty value fits any type (the field is not filled).
    ** - TEXT — any text as is (the edges are not touched: spaces may be a part of the value);
    ** - NUMBER — [+|-]digits[(.|,)digits][e[+|-]digits]; a comma becomes a dot, "+" is removed;
    ** - DATE — YYYY-MM-DD or DD.MM.YYYY (the day and the month — one or two digits) → YYYY-MM-DD, the date exists;
    ** - BOOLEAN — true/false, 1/0, yes/no, on/off, да/нет (Latin case-insensitive) → "true" / "false".
    ** false — the text does not fit the type (text is not changed).
    */
    bool normalizeValue(ColumnType type, std::string& text);

    // Логическое значение истинно: то, что normalizeValue(BOOLEAN) приводит к «true» / The boolean value is true: what normalizeValue(BOOLEAN) brings to "true"
    bool valueTrue(std::string_view text);
}

#endif
