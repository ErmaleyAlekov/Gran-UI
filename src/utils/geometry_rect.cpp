#include "gran/utils/geometry.hpp"

#include <algorithm>

namespace gran
{
    // Левая и верхняя границы входят, правая и нижняя — нет (как у пикселей)
    // The left and top edges are included, the right and bottom ones are not (as with pixels)
    bool rectContains(const Rect& rect, Vec2 point) noexcept
    {
        return point.x >= rect.x && point.y >= rect.y
            && point.x < rect.x + rect.width && point.y < rect.y + rect.height;
    }

    // Пересечение по каждой оси; без перекрытия — нулевой размер
    // The intersection on each axis; without an overlap — a zero size
    Rect rectIntersect(const Rect& first, const Rect& second) noexcept
    {
        float left = std::max(first.x, second.x);
        float top = std::max(first.y, second.y);
        float right = std::min(first.x + first.width, second.x + second.width);
        float bottom = std::min(first.y + first.height, second.y + second.height);
        Rect result{left, top, 0.0f, 0.0f};

        if (right > left && bottom > top)
        {
            result.width = right - left;
            result.height = bottom - top;
        }
        return result;
    }

    // Пустой прямоугольник не расширяет объединение
    // An empty rectangle does not extend the union
    Rect rectUnion(const Rect& first, const Rect& second) noexcept
    {
        if (rectIsEmpty(first))
            return second;
        if (rectIsEmpty(second))
            return first;
        float left = std::min(first.x, second.x);
        float top = std::min(first.y, second.y);
        float right = std::max(first.x + first.width, second.x + second.width);
        float bottom = std::max(first.y + first.height, second.y + second.height);
        return Rect{left, top, right - left, bottom - top};
    }

    // Отступы «съедают» размер, но не делают его отрицательным
    // The insets "eat" the size but never make it negative
    Rect rectDeflate(const Rect& rect, const Insets& insets) noexcept
    {
        Rect result{rect.x + insets.left, rect.y + insets.top, 0.0f, 0.0f};

        result.width = std::max(0.0f, rect.width - insets.left - insets.right);
        result.height = std::max(0.0f, rect.height - insets.top - insets.bottom);
        return result;
    }

    // Пустой — ширина или высота не положительна
    // Empty — the width or the height is not positive
    bool rectIsEmpty(const Rect& rect) noexcept
    {
        return rect.width <= 0.0f || rect.height <= 0.0f;
    }
}
