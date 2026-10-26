#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/utils/geometry.hpp"

namespace
{
    // Точка внутри прямоугольника (правая и нижняя границы не входят)
    // The point is inside the rectangle (the right and bottom edges are excluded)
    void testRectContains()
    {
        gran::Rect rect{0, 0, 10, 10};
        TEST_ASSERT(gran::rectContains(rect, {5, 5}));
        TEST_ASSERT(gran::rectContains(rect, {0, 0}));
        TEST_ASSERT(!gran::rectContains(rect, {10, 0}));
        TEST_ASSERT(!gran::rectContains(rect, {0, 10}));
    }

    // Пересечение; пустое — прямоугольник нулевого размера
    // The intersection; an empty one is a zero size rectangle
    void testRectIntersect()
    {
        gran::Rect rect1{0, 0, 10, 10};
        gran::Rect rect2{5, 5, 10, 10};
        gran::Rect result = gran::rectIntersect(rect1, rect2);
        TEST_ASSERT_NEAR(result.x, 5.0f);
        TEST_ASSERT_NEAR(result.y, 5.0f);
        TEST_ASSERT_NEAR(result.width, 5.0f);
        TEST_ASSERT_NEAR(result.height, 5.0f);

        gran::Rect rect3{10, 10, 10, 10};
        result = gran::rectIntersect(rect1, rect3);
        TEST_ASSERT_NEAR(result.width, 0.0f);
        TEST_ASSERT_NEAR(result.height, 0.0f);
    }

    // Наименьший прямоугольник, содержащий оба / The smallest rectangle containing both
    void testRectUnion()
    {
        gran::Rect rect1{0, 0, 10, 10};
        gran::Rect rect2{5, 5, 10, 10};
        gran::Rect result = gran::rectUnion(rect1, rect2);
        TEST_ASSERT_NEAR(result.x, 0.0f);
        TEST_ASSERT_NEAR(result.y, 0.0f);
        TEST_ASSERT_NEAR(result.width, 15.0f);
        TEST_ASSERT_NEAR(result.height, 15.0f);

        gran::Rect rect3{15, 15, 10, 10};
        result = gran::rectUnion(rect1, rect3);
        TEST_ASSERT_NEAR(result.x, 0.0f);
        TEST_ASSERT_NEAR(result.y, 0.0f);
        TEST_ASSERT_NEAR(result.width, 25.0f);
        TEST_ASSERT_NEAR(result.height, 25.0f);
    }

    // Прямоугольник, уменьшенный на отступы (размер не меньше 0)
    // The rectangle shrunk by the insets (the size is not below 0)
    void testRectDeflate()
    {
        gran::Rect rect{0, 0, 10, 10};
        gran::Insets insets{1, 1, 1, 1};
        gran::Rect result = gran::rectDeflate(rect, insets);
        TEST_ASSERT_NEAR(result.x, 1.0f);
        TEST_ASSERT_NEAR(result.y, 1.0f);
        TEST_ASSERT_NEAR(result.width, 8.0f);
        TEST_ASSERT_NEAR(result.height, 8.0f);

        gran::Insets insets2{5, 5, 5, 5};
        result = gran::rectDeflate(rect, insets2);
        TEST_ASSERT_NEAR(result.width, 0.0f);
        TEST_ASSERT_NEAR(result.height, 0.0f);
    }

    // Композиция: сначала second, затем first (first · second)
    // The composition: second first, then first (first · second)
    void testAffineMultiply()
    {
        gran::Affine2D first{2, 0, 0, 2, 0, 0};
        gran::Affine2D second{1, 0, 0, 1, 10, 10};
        gran::Affine2D result = gran::affineMultiply(first, second);
        TEST_ASSERT_NEAR(result.a, 2.0f);
        TEST_ASSERT_NEAR(result.b, 0.0f);
        TEST_ASSERT_NEAR(result.c, 0.0f);
        TEST_ASSERT_NEAR(result.d, 2.0f);
        TEST_ASSERT_NEAR(result.tx, 20.0f);
        TEST_ASSERT_NEAR(result.ty, 20.0f);
    }

    // Применение к точке / Applying to a point
    void testAffineApply()
    {
        gran::Affine2D transform{2, 0, 0, 2, 10, 10};
        gran::Vec2 point{5, 5};
        gran::Vec2 result = gran::affineApply(transform, point);
        TEST_ASSERT_NEAR(result.x, 20.0f);
        TEST_ASSERT_NEAR(result.y, 20.0f);
    }

    // Обратное преобразование; false — вырожденное (определитель ≈ 0)
    // The inverse transform; false — degenerate (the determinant ≈ 0)
    void testAffineInvert()
    {
        gran::Affine2D transform{2, 0, 0, 2, 0, 0};
        gran::Affine2D inverse;
        TEST_ASSERT(gran::affineInvert(transform, inverse));
        TEST_ASSERT_NEAR(inverse.a, 0.5f);
        TEST_ASSERT_NEAR(inverse.b, 0.0f);
        TEST_ASSERT_NEAR(inverse.c, 0.0f);
        TEST_ASSERT_NEAR(inverse.d, 0.5f);
        TEST_ASSERT_NEAR(inverse.tx, 0.0f);
        TEST_ASSERT_NEAR(inverse.ty, 0.0f);

        gran::Affine2D degenerate{0, 0, 0, 0, 0, 0};
        TEST_ASSERT(!gran::affineInvert(degenerate, inverse));
    }

    // Описанный прямоугольник после преобразования / The bounding rectangle after the transform
    void testAffineBounds()
    {
        gran::Affine2D transform{0, 1, -1, 0, 0, 0};
        gran::Rect rect{0, 0, 10, 20};
        gran::Rect result = gran::affineBounds(transform, rect);
        TEST_ASSERT_NEAR(result.width, 20.0f);
        TEST_ASSERT_NEAR(result.height, 10.0f);
    }
}

namespace suites
{
    // Регистрация набора / The suite registration
    void registerGeometryTests(test::TestRunner& runner)
    {
        runner.add("geometry: содержит точку", testRectContains);
        runner.add("geometry: пересечение", testRectIntersect);
        runner.add("geometry: объединение", testRectUnion);
        runner.add("geometry: сжатие", testRectDeflate);
        runner.add("geometry: умножение преобразований", testAffineMultiply);
        runner.add("geometry: применение преобразования", testAffineApply);
        runner.add("geometry: инверсия преобразования", testAffineInvert);
        runner.add("geometry: границы преобразования", testAffineBounds);
    }
}
