#include "gran/style/style_compare.hpp"

#include <cmath>

namespace gran::style
{
    // NaN означает «не задано»: два незаданных значения равны / NaN means "unset": two unset values are equal
    bool sameFloat(float first, float second) noexcept
    {
        if (std::isnan(first) || std::isnan(second))
            return std::isnan(first) && std::isnan(second);
        return first == second;
    }

    // Единица и значение / The unit and the value
    bool sameLength(const Length& first, const Length& second) noexcept
    {
        return first.unit == second.unit && first.value == second.value;
    }

    // Все четыре стороны / All four sides
    bool sameInsets(const Insets& first, const Insets& second) noexcept
    {
        return first.top == second.top && first.right == second.right && first.bottom == second.bottom
            && first.left == second.left;
    }

    // Поэлементно: вид и значение обеих границ / Element by element: the kind and the value of both bounds
    bool sameTracks(const std::vector<GridTrack>& first, const std::vector<GridTrack>& second) noexcept
    {
        if (first.size() != second.size())
            return false;
        for (std::size_t index = 0; index < first.size(); ++index)
        {
            const GridTrack& a = first[index];
            const GridTrack& b = second[index];
            if (a.min.kind != b.min.kind || a.min.value != b.min.value || a.max.kind != b.max.kind
                || a.max.value != b.max.value)
                return false;
        }
        return true;
    }
}
