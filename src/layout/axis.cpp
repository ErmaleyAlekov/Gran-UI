#include "gran/layout/internal/layout_internal.hpp"

namespace gran::layout::internal
{
    // Ширина для горизонтали, высота для вертикали / The width for horizontal, the height for vertical
    float mainOf(Size size, bool horizontal) noexcept
    {
        if (horizontal)
            return size.width;
        return size.height;
    }

    // Противоположная компонента / The opposite component
    float crossOf(Size size, bool horizontal) noexcept
    {
        if (horizontal)
            return size.height;
        return size.width;
    }

    // Сборка размера из компонент осей / Building a size from the axis components
    Size sizeFrom(float main, float cross, bool horizontal) noexcept
    {
        if (horizontal)
            return Size{main, cross};
        return Size{cross, main};
    }

    // Отступы вдоль главной оси / The insets along the main axis
    float insetsMain(const Insets& insets, bool horizontal) noexcept
    {
        if (horizontal)
            return insets.left + insets.right;
        return insets.top + insets.bottom;
    }

    // Отступы вдоль поперечной оси / The insets along the cross axis
    float insetsCross(const Insets& insets, bool horizontal) noexcept
    {
        if (horizontal)
            return insets.top + insets.bottom;
        return insets.left + insets.right;
    }
}
