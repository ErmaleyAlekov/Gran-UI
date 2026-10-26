#ifndef GRAN_UTILS_GEOMETRY_HPP
#define GRAN_UTILS_GEOMETRY_HPP

namespace gran
{
    // Точка или вектор на плоскости (логические пиксели)
    // A point or a vector on the plane (logical pixels)
    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    // Размер прямоугольной области
    // The size of a rectangular area
    struct Size
    {
        float width = 0.0f;
        float height = 0.0f;
    };

    // Прямоугольник: левый верхний угол и размер
    // A rectangle: the top left corner and the size
    struct Rect
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    // Отступы по сторонам в порядке CSS: верх, право, низ, лево
    // Side insets in the CSS order: top, right, bottom, left
    struct Insets
    {
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
        float left = 0.0f;
    };

    // Радиусы скругления углов по часовой стрелке от левого верхнего
    // Corner rounding radii clockwise from the top left one
    struct CornerRadii
    {
        float topLeft = 0.0f;
        float topRight = 0.0f;
        float bottomRight = 0.0f;
        float bottomLeft = 0.0f;
    };

    /*
    ** Аффинное преобразование 2D: x' = a·x + c·y + tx, y' = b·x + d·y + ty
    ** (столбцы как у матрицы CSS matrix(a, b, c, d, tx, ty)).
    **
    ** A 2D affine transform: x' = a·x + c·y + tx, y' = b·x + d·y + ty
    ** (the columns are as in the CSS matrix(a, b, c, d, tx, ty)).
    */
    struct Affine2D
    {
        float a = 1.0f;
        float b = 0.0f;
        float c = 0.0f;
        float d = 1.0f;
        float tx = 0.0f;
        float ty = 0.0f;
    };

    // ---- Прямоугольники (geometry_rect.cpp) / Rectangles (geometry_rect.cpp) ----

    // Точка внутри прямоугольника (правая и нижняя границы не входят)
    // The point is inside the rectangle (the right and bottom edges are excluded)
    bool rectContains(const Rect& rect, Vec2 point) noexcept;

    // Пересечение; пустое — прямоугольник нулевого размера
    // The intersection; an empty one is a zero size rectangle
    Rect rectIntersect(const Rect& first, const Rect& second) noexcept;

    // Наименьший прямоугольник, содержащий оба / The smallest rectangle containing both
    Rect rectUnion(const Rect& first, const Rect& second) noexcept;

    // Прямоугольник, уменьшенный на отступы (размер не меньше 0)
    // The rectangle shrunk by the insets (the size is not below 0)
    Rect rectDeflate(const Rect& rect, const Insets& insets) noexcept;

    // Прямоугольник пуст (ширина или высота ≤ 0) / The rectangle is empty (width or height ≤ 0)
    bool rectIsEmpty(const Rect& rect) noexcept;

    // ---- Преобразования (geometry_affine.cpp) / Transforms (geometry_affine.cpp) ----

    // Композиция: сначала second, затем first (first · second)
    // The composition: second first, then first (first · second)
    Affine2D affineMultiply(const Affine2D& first, const Affine2D& second) noexcept;

    // Применение к точке / Applying to a point
    Vec2 affineApply(const Affine2D& transform, Vec2 point) noexcept;

    // Обратное преобразование; false — вырожденное (определитель ≈ 0)
    // The inverse transform; false — degenerate (the determinant ≈ 0)
    bool affineInvert(const Affine2D& transform, Affine2D& inverse) noexcept;

    // Преобразование тождественное / The transform is the identity
    bool affineIsIdentity(const Affine2D& transform) noexcept;

    // Описанный прямоугольник после преобразования / The bounding rectangle after the transform
    Rect affineBounds(const Affine2D& transform, const Rect& rect) noexcept;
}

#endif
