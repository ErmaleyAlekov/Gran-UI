#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/vector/shape.hpp"

namespace
{
    void testParsePathData()
    {
        auto path = gran::vector::parsePathData("M1 2 L3 4 Z");
        TEST_ASSERT(path.verbs.size() == 3);
        TEST_ASSERT(path.verbs[0] == gran::vector::Verb::MOVE);
        TEST_ASSERT(path.verbs[1] == gran::vector::Verb::LINE);
        TEST_ASSERT(path.verbs[2] == gran::vector::Verb::CLOSE);
        TEST_ASSERT(path.points.size() == 2);
        TEST_ASSERT(path.points[0].x == 1.0f);
        TEST_ASSERT(path.points[0].y == 2.0f);
        TEST_ASSERT(path.points[1].x == 3.0f);
        TEST_ASSERT(path.points[1].y == 4.0f);

        path = gran::vector::parsePathData("m1 1 h2 v3 l-1 0");
        TEST_ASSERT(path.verbs.size() == 4);
        TEST_ASSERT(path.verbs[0] == gran::vector::Verb::MOVE);
        TEST_ASSERT(path.verbs[1] == gran::vector::Verb::LINE);
        TEST_ASSERT(path.verbs[2] == gran::vector::Verb::LINE);
        TEST_ASSERT(path.verbs[3] == gran::vector::Verb::LINE);
        TEST_ASSERT(path.points.size() == 4);
        TEST_ASSERT(path.points[0].x == 1.0f);
        TEST_ASSERT(path.points[0].y == 1.0f);
        TEST_ASSERT(path.points[1].x == 3.0f);
        TEST_ASSERT(path.points[1].y == 1.0f);
        TEST_ASSERT(path.points[2].x == 3.0f);
        TEST_ASSERT(path.points[2].y == 4.0f);
        TEST_ASSERT(path.points[3].x == 2.0f);
        TEST_ASSERT(path.points[3].y == 4.0f);

        path = gran::vector::parsePathData("M0 0 1 1 2 2");
        TEST_ASSERT(path.verbs.size() == 3);
        TEST_ASSERT(path.verbs[0] == gran::vector::Verb::MOVE);
        TEST_ASSERT(path.verbs[1] == gran::vector::Verb::LINE);
        TEST_ASSERT(path.verbs[2] == gran::vector::Verb::LINE);
        TEST_ASSERT(path.points.size() == 3);
        TEST_ASSERT(path.points[0].x == 0.0f);
        TEST_ASSERT(path.points[0].y == 0.0f);
        TEST_ASSERT(path.points[1].x == 1.0f);
        TEST_ASSERT(path.points[1].y == 1.0f);
        TEST_ASSERT(path.points[2].x == 2.0f);
        TEST_ASSERT(path.points[2].y == 2.0f);

        path = gran::vector::parsePathData("M1.5.5-1-2");
        TEST_ASSERT(path.verbs.size() == 2);
        TEST_ASSERT(path.verbs[0] == gran::vector::Verb::MOVE);
        TEST_ASSERT(path.verbs[1] == gran::vector::Verb::LINE);
        TEST_ASSERT(path.points.size() == 2);
        TEST_ASSERT(path.points[0].x == 1.5f);
        TEST_ASSERT(path.points[0].y == 0.5f);
        TEST_ASSERT(path.points[1].x == -1.0f);
        TEST_ASSERT(path.points[1].y == -2.0f);

        path = gran::vector::parsePathData("M0 0 C1 1 2 1 3 0 S5 -1 6 0");
        TEST_ASSERT(path.verbs.size() == 3);
        TEST_ASSERT(path.verbs[0] == gran::vector::Verb::MOVE);
        TEST_ASSERT(path.verbs[1] == gran::vector::Verb::CUBIC);
        TEST_ASSERT(path.verbs[2] == gran::vector::Verb::CUBIC);
        TEST_ASSERT(path.points.size() == 7);
        TEST_ASSERT(path.points[4].x == 4.0f);
        TEST_ASSERT(path.points[4].y == -1.0f);

        path = gran::vector::parsePathData("M0 0 A5 5 0 0110 0");
        // Полуокружность — два куска по 90° / A half circle is two 90° pieces
        TEST_ASSERT(path.verbs.size() == 3);
        TEST_ASSERT(path.verbs[0] == gran::vector::Verb::MOVE);
        TEST_ASSERT(path.verbs.back() == gran::vector::Verb::CUBIC);
        TEST_ASSERT(path.points.back().x == 10.0f);
        TEST_ASSERT(path.points.back().y == 0.0f);

        TEST_ASSERT_THROWS(gran::vector::parsePathData("L1 1"), gran::GranException);
        TEST_ASSERT_THROWS(gran::vector::parsePathData("M0 0 L1"), gran::GranException);
        TEST_ASSERT_THROWS(gran::vector::parsePathData("M0 0 X1 1"), gran::GranException);
        TEST_ASSERT_THROWS(gran::vector::parsePathData("M0 0 Z 1 1"), gran::GranException);
    }

    void testParseViewBox()
    {
        gran::Rect rect;
        TEST_ASSERT(gran::vector::parseViewBox("0 0 24 24", rect));
        TEST_ASSERT(rect.width == 24.0f);
        TEST_ASSERT(rect.height == 24.0f);

        TEST_ASSERT(gran::vector::parseViewBox("0,0,16,8", rect));
        TEST_ASSERT(rect.width == 16.0f);
        TEST_ASSERT(rect.height == 8.0f);

        TEST_ASSERT(!gran::vector::parseViewBox("0 0 0 24", rect));
        TEST_ASSERT(!gran::vector::parseViewBox("0 0 24", rect));
    }

    // Покрытие в точке (x, y) фигуры: у растра есть поле, его угол — (left, top) / The coverage at the shape point (x, y): the raster has a margin, its corner is (left, top)
    int coverageAt(const gran::text::GlyphBitmap& bitmap, int x, int y)
    {
        return bitmap.coverage[static_cast<std::size_t>((y - bitmap.top) * bitmap.width + (x - bitmap.left))];
    }

    void testRasterizeShape()
    {
        gran::vector::Shape shape;
        shape.viewBox = gran::Rect{0, 0, 24, 24};
        shape.paths.push_back(gran::vector::parsePathData("M0 0H24V24H0Z"));

        gran::text::GlyphBitmap bitmap;
        TEST_ASSERT(gran::vector::rasterizeShape(shape, 24, 24, bitmap));
        TEST_ASSERT(bitmap.coverage.size() == static_cast<std::size_t>(bitmap.width * bitmap.height));
        TEST_ASSERT(bitmap.width >= 24 && bitmap.height >= 24);
        TEST_ASSERT(coverageAt(bitmap, 12, 12) == 255);
        TEST_ASSERT(coverageAt(bitmap, bitmap.left, bitmap.top) == 0);
    }

    void testEvenOdd()
    {
        gran::vector::Shape shape;
        shape.viewBox = gran::Rect{0, 0, 24, 24};
        shape.paths.push_back(gran::vector::parsePathData("M0 0H24V24H0Z M6 6H18V18H6Z"));

        gran::text::GlyphBitmap bitmap;
        shape.paths[0].evenOdd = true;
        TEST_ASSERT(gran::vector::rasterizeShape(shape, 24, 24, bitmap));
        TEST_ASSERT(coverageAt(bitmap, 12, 12) < 10);
        TEST_ASSERT(coverageAt(bitmap, 3, 3) == 255);
        shape.paths[0].evenOdd = false;
        TEST_ASSERT(gran::vector::rasterizeShape(shape, 24, 24, bitmap));
        TEST_ASSERT(coverageAt(bitmap, 12, 12) == 255);
    }
}

namespace suites
{
    void registerVectorShapeTests(test::TestRunner& runner)
    {
        runner.add("vector: parsePathData", testParsePathData);
        runner.add("vector: parseViewBox", testParseViewBox);
        runner.add("vector: rasterizeShape", testRasterizeShape);
        runner.add("vector: evenOdd", testEvenOdd);
    }
}
