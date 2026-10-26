#ifndef GRAN_STYLE_INTERNAL_NAMED_COLORS_HPP
#define GRAN_STYLE_INTERNAL_NAMED_COLORS_HPP

#include "gran/utils/color.hpp"

#include <string_view>

namespace gran::style::internal
{
    // Именованный цвет CSS (без учёта регистра) и "transparent"; false — имени нет
    // A CSS named color (case-insensitive) and "transparent"; false — no such name
    bool findNamedColor(std::string_view name, Color& color) noexcept;
}

#endif
