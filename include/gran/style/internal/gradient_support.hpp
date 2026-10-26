#ifndef GRAN_STYLE_INTERNAL_GRADIENT_SUPPORT_HPP
#define GRAN_STYLE_INTERNAL_GRADIENT_SUPPORT_HPP

#include "gran/style/values.hpp"

#include <string_view>
#include <vector>

namespace gran::style::internal
{
    /*
    ** Опорные точки из аргументов функции начиная с first: "цвет [поз [поз]]".
    ** Нужно не меньше двух точек; позиции без значения расставляются равномерно,
    ** убывающая позиция поднимается до предыдущей (правило CSS).
    **
    ** The stops from the function arguments starting at first: "color [pos [pos]]".
    ** At least two stops are needed; positions without a value are spread evenly,
    ** a decreasing position is raised to the previous one (the CSS rule).
    */
    std::vector<GradientStop> parseStops(const std::vector<std::string_view>& arguments,
        std::size_t first, std::string_view text);

    // Центр "at x y": проценты или left/center/right, top/center/bottom
    // The center "at x y": percent or left/center/right, top/center/bottom
    Vec2 parseGradientCenter(std::string_view text, std::string_view original);
}

#endif
