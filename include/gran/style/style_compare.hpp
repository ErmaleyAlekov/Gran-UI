#ifndef GRAN_STYLE_STYLE_COMPARE_HPP
#define GRAN_STYLE_STYLE_COMPARE_HPP

#include "gran/style/computed_style.hpp"

namespace gran::style
{
    // ---- style_compare_values.cpp ----

    // Равенство чисел, где два NaN («не задано») равны / Number equality where two NaNs ("unset") are equal
    bool sameFloat(float first, float second) noexcept;

    // Равенство длин / Length equality
    bool sameLength(const Length& first, const Length& second) noexcept;

    // Равенство отступов / Insets equality
    bool sameInsets(const Insets& first, const Insets& second) noexcept;

    // Равенство списков дорожек сетки / Grid track list equality
    bool sameTracks(const std::vector<GridTrack>& first, const std::vector<GridTrack>& second) noexcept;

    // ---- style_compare.cpp ----

    /*
    ** Влияет ли смена стиля на компоновку (размеры, отступы, расположение детей, метрики
    ** текста). Цвет, тень, прозрачность, преобразование — нет: такие изменения только перерисовывают.
    **
    ** Whether a style change affects the layout (sizes, insets, children arrangement, text
    ** metrics). Color, shadow, opacity, transform do not: such changes only repaint.
    */
    bool affectsLayout(const ComputedStyle& before, const ComputedStyle& after) noexcept;
}

#endif
