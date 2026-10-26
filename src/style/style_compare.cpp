#include "gran/style/style_compare.hpp"

namespace gran::style
{
    namespace
    {
        // Размер и место самого блока / The size and the place of the box itself
        bool sameBox(const ComputedStyle& a, const ComputedStyle& b) noexcept
        {
            return sameLength(a.width, b.width) && sameLength(a.height, b.height) && a.minWidth == b.minWidth
                && a.maxWidth == b.maxWidth && a.minHeight == b.minHeight && a.maxHeight == b.maxHeight
                && sameInsets(a.margin, b.margin) && sameInsets(a.padding, b.padding)
                && sameInsets(a.borderWidth, b.borderWidth) && a.grow == b.grow && a.shrink == b.shrink
                && a.alignSelf == b.alignSelf && a.gridRow == b.gridRow && a.gridColumn == b.gridColumn
                && a.gridRowSpan == b.gridRowSpan && a.gridColumnSpan == b.gridColumnSpan
                && sameFloat(a.left, b.left) && sameFloat(a.top, b.top) && sameFloat(a.right, b.right)
                && sameFloat(a.bottom, b.bottom);
        }

        // Расположение детей / The children arrangement
        bool sameFlow(const ComputedStyle& a, const ComputedStyle& b) noexcept
        {
            return a.orientation == b.orientation && a.spacing == b.spacing && a.justify == b.justify
                && a.align == b.align && a.wrap == b.wrap && sameTracks(a.rows, b.rows)
                && sameTracks(a.columns, b.columns) && a.rowSpacing == b.rowSpacing
                && a.columnSpacing == b.columnSpacing;
        }

        // Метрики текста (цвет не входит) / The text metrics (the color is not included)
        bool sameText(const ComputedStyle& a, const ComputedStyle& b) noexcept
        {
            return a.fontFamily == b.fontFamily && a.fontSize == b.fontSize && a.fontWeight == b.fontWeight
                && a.fontStyle == b.fontStyle && a.lineHeight == b.lineHeight && a.letterSpacing == b.letterSpacing
                && a.textWrap == b.textWrap && a.maxLines == b.maxLines;
        }
    }

    // Изменение хотя бы одной группы требует компоновки / A change in at least one group requires a layout
    bool affectsLayout(const ComputedStyle& before, const ComputedStyle& after) noexcept
    {
        return !sameBox(before, after) || !sameFlow(before, after) || !sameText(before, after);
    }
}
