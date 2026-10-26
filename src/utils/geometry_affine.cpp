#include "gran/utils/geometry.hpp"

#include <algorithm>
#include <cmath>

namespace gran
{
    // Произведение матриц 3×3 с последней строкой (0, 0, 1)
    // The product of 3×3 matrices with the last row (0, 0, 1)
    Affine2D affineMultiply(const Affine2D& first, const Affine2D& second) noexcept
    {
        Affine2D result;

        result.a = first.a * second.a + first.c * second.b;
        result.b = first.b * second.a + first.d * second.b;
        result.c = first.a * second.c + first.c * second.d;
        result.d = first.b * second.c + first.d * second.d;
        result.tx = first.a * second.tx + first.c * second.ty + first.tx;
        result.ty = first.b * second.tx + first.d * second.ty + first.ty;
        return result;
    }

    // Точка после преобразования / The point after the transform
    Vec2 affineApply(const Affine2D& transform, Vec2 point) noexcept
    {
        return Vec2{transform.a * point.x + transform.c * point.y + transform.tx,
            transform.b * point.x + transform.d * point.y + transform.ty};
    }

    // Обращение через определитель; почти нулевой определитель — отказ
    // Inversion through the determinant; an almost zero determinant is a refusal
    bool affineInvert(const Affine2D& transform, Affine2D& inverse) noexcept
    {
        float determinant = transform.a * transform.d - transform.b * transform.c;

        if (std::fabs(determinant) < 1e-12f)
            return false;
        float scale = 1.0f / determinant;
        inverse.a = transform.d * scale;
        inverse.b = -transform.b * scale;
        inverse.c = -transform.c * scale;
        inverse.d = transform.a * scale;
        inverse.tx = -(inverse.a * transform.tx + inverse.c * transform.ty);
        inverse.ty = -(inverse.b * transform.tx + inverse.d * transform.ty);
        return true;
    }

    // Точное сравнение: тождество задаётся явно, а не получается вычислением
    // An exact comparison: the identity is set explicitly, not obtained by computation
    bool affineIsIdentity(const Affine2D& transform) noexcept
    {
        return transform.a == 1.0f && transform.b == 0.0f && transform.c == 0.0f
            && transform.d == 1.0f && transform.tx == 0.0f && transform.ty == 0.0f;
    }

    // Описанный прямоугольник по четырём преобразованным углам
    // The bounding rectangle over the four transformed corners
    Rect affineBounds(const Affine2D& transform, const Rect& rect) noexcept
    {
        Vec2 corners[4] = {
            affineApply(transform, Vec2{rect.x, rect.y}),
            affineApply(transform, Vec2{rect.x + rect.width, rect.y}),
            affineApply(transform, Vec2{rect.x + rect.width, rect.y + rect.height}),
            affineApply(transform, Vec2{rect.x, rect.y + rect.height})};
        Vec2 low = corners[0];
        Vec2 high = corners[0];

        for (const Vec2& corner : corners)
        {
            low.x = std::min(low.x, corner.x);
            low.y = std::min(low.y, corner.y);
            high.x = std::max(high.x, corner.x);
            high.y = std::max(high.y, corner.y);
        }
        return Rect{low.x, low.y, high.x - low.x, high.y - low.y};
    }
}
