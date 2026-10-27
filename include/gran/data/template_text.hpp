#ifndef GRAN_DATA_TEMPLATE_TEXT_HPP
#define GRAN_DATA_TEMPLATE_TEXT_HPP

#include "gran/data/list_model.hpp"

#include <string>
#include <string_view>

namespace gran::data
{
    // ---- template_text.cpp ----

    // Есть ли в тексте подстановка {поле} (двойные {{ и }} — не подстановка) / Whether the text has a {field} placeholder ({{ and }} are not one)
    bool hasPlaceholders(std::string_view pattern) noexcept;

    /*
    ** Текст шаблона с подставленными полями записи: «{name}» → значение поля name,
    ** «{{» → «{», «}}» → «}». Нет такого поля — пустая строка (как пустой атрибут);
    ** незакрытая «{» остаётся как есть.
    **
    ** The template text with the record fields substituted: "{name}" → the value of the name field,
    ** "{{" → "{", "}}" → "}". No such field — an empty string (like an empty attribute);
    ** an unclosed "{" stays as it is.
    */
    std::string expandTemplate(std::string_view pattern, const Record& record);
}

#endif
