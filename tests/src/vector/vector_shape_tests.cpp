#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/render/image_store.hpp"
#include "gran/res/resources.hpp"
#include "gran/rhi/null/null_backend.hpp"
#include "gran/vector/shape.hpp"
#include <cmath>

namespace
{
    void mountSvg(const char* name, std::string_view svg)
    {
        const gran::res::Entry entry{name, reinterpret_cast<const std::uint8_t*>(svg.data()), svg.size()};
        gran::res::mount({"svg-fixtures", &entry, 1});
    }

    void testSvgDimensions()
    {
        mountSvg("svg-tests/view-box.svg", R"(<svg viewBox="1 2 30 40" width="0" height="-10"/>)");
        mountSvg("svg-tests/width.svg", R"(<svg width="18"/>)");
        mountSvg("svg-tests/height.svg", R"(<svg height="19" viewBox="0 0 0 20"/>)");
        const auto view = gran::vector::loadSvgFile("res:/svg-tests/view-box.svg")->viewBox;
        TEST_ASSERT(view.x == 1 && view.y == 2 && view.width == 30 && view.height == 40);
        const auto width = gran::vector::loadSvgFile("res:/svg-tests/width.svg")->viewBox;
        TEST_ASSERT(width.width == 18 && width.height == 24);
        const auto height = gran::vector::loadSvgFile("res:/svg-tests/height.svg")->viewBox;
        TEST_ASSERT(height.width == 24 && height.height == 19);
    }

    void testInvalidSvgDimensions()
    {
        constexpr const char* resource = "svg-tests/invalid-size.svg";
        constexpr const char* path = "res:/svg-tests/invalid-size.svg";
        for (const auto svg : {R"(<svg width="0" height="24"/>)", R"(<svg width="24" height="-1"/>)"})
        {
            mountSvg(resource, svg);
            bool rejected = false;
            try { gran::vector::loadSvgFile(path); }
            catch (const gran::GranException& error) { rejected = error.getCode() == gran::ErrorCode::MARKUP; }
            TEST_ASSERT(rejected);
        }
        gran::rhi::NullBackend backend;
        gran::render::ImageStore cache(backend, {});
        TEST_ASSERT(cache.shape(path) == nullptr);
        mountSvg(resource, R"(<svg width="24" height="24"><rect width="10" height="10"/></svg>)");
        TEST_ASSERT(gran::vector::loadSvgFile(path) != nullptr);
        TEST_ASSERT(cache.shape(path) == nullptr);
    }

    void testSvgRectRadii()
    {
        mountSvg("svg-tests/rect-radii.svg", R"(<svg viewBox="0 0 10 10"><rect width="10" height="10" rx="0" ry="3"/>
            <rect width="10" height="10" rx="2"/><rect width="10" height="10"/></svg>)");
        const auto shape = gran::vector::loadSvgFile("res:/svg-tests/rect-radii.svg");
        TEST_ASSERT(shape->paths.size() == 3);
        gran::vector::Shape single;
        single.viewBox = shape->viewBox;
        single.paths = {shape->paths[0]};
        gran::text::GlyphBitmap bitmap;
        TEST_ASSERT(gran::vector::rasterizeShape(single, 10, 10, bitmap));
        single.paths = {shape->paths[2]};
        gran::text::GlyphBitmap plain;
        TEST_ASSERT(gran::vector::rasterizeShape(single, 10, 10, plain));
        TEST_ASSERT(bitmap.width == plain.width && bitmap.height == plain.height);
        TEST_ASSERT(bitmap.left == plain.left && bitmap.top == plain.top && bitmap.coverage == plain.coverage);
        const auto& rounded = shape->paths[1].points;
        TEST_ASSERT(rounded.size() > 4);
        TEST_ASSERT(rounded[0].x == 2 && rounded[4].x == 10 && rounded[4].y == 2);

        single.viewBox.width = 0;
        TEST_ASSERT(!gran::vector::rasterizeShape(single, 10, 10, bitmap) && bitmap.coverage.empty());
        single.viewBox.width = 10;
        single.viewBox.height = 0;
        TEST_ASSERT(!gran::vector::rasterizeShape(single, 10, 10, bitmap) && bitmap.coverage.empty());
    }

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
        TEST_ASSERT_THROWS(gran::vector::parsePathData("M0 0 L1e500 0"), gran::GranException);
    }

    void testArcExtremeRadii()
    {
        for (const auto* input : {"M0 0 A1e20 1e20 0 0 1 100 0", "M0 0 A1e-25 1e-25 0 0 1 100 0"})
        {
            const auto path = gran::vector::parsePathData(input);
            TEST_ASSERT(path.verbs.size() >= 2 && path.verbs.size() <= 5);
            TEST_ASSERT(path.points.back().x == 100.0f && path.points.back().y == 0.0f);
            for (const auto& point : path.points)
                TEST_ASSERT(std::isfinite(point.x) && std::isfinite(point.y));
        }
    }

    void testShallowRelativeArc()
    {
        const auto path = gran::vector::parsePathData("M10 20 a1e20 1e20 0 0 1 90 0");
        TEST_ASSERT(path.verbs.size() == 2 && path.verbs.back() == gran::vector::Verb::CUBIC);
        TEST_ASSERT(path.points.size() == 4);
        TEST_ASSERT_NEAR(path.points[1].x, 40.0f);
        TEST_ASSERT_NEAR(path.points[2].x, 70.0f);
        TEST_ASSERT_NEAR(path.points[1].y, 20.0f);
        TEST_ASSERT_NEAR(path.points[2].y, 20.0f);
        TEST_ASSERT(path.points.back().x == 100 && path.points.back().y == 20);
    }

    void testArcRotationAndFollowingCurve()
    {
        const auto orthogonal = gran::vector::parsePathData("M0 0 A1e20 1e-20 90 0 1 0 100");
        TEST_ASSERT(orthogonal.points.size() == 4);
        TEST_ASSERT_NEAR(orthogonal.points[1].x, 0.0f);
        TEST_ASSERT_NEAR(orthogonal.points[1].y, 100.0f / 3);

        const auto diagonal = gran::vector::parsePathData("M0 0 A1e20 1e-20 45 0 1 100 100");
        TEST_ASSERT(diagonal.points.size() == 4);
        TEST_ASSERT_NEAR(diagonal.points[1].x, 100.0f / 3);
        TEST_ASSERT_NEAR(diagonal.points[1].y, 100.0f / 3);
        TEST_ASSERT_NEAR(diagonal.points[2].x, 200.0f / 3);
        TEST_ASSERT_NEAR(diagonal.points[2].y, 200.0f / 3);

        const auto smooth = gran::vector::parsePathData("M0 0 A5 5 0 0 1 10 0 S12 2 15 0");
        const auto& control = smooth.points[smooth.points.size() - 3];
        TEST_ASSERT(control.x == 10 && control.y == 0);
    }

    void testArcOutOfRange()
    {
        bool badPath = false;
        try
        {
            gran::vector::parsePathData("M0 0 A3e38 3e38 0 1 1 100 0");
        }
        catch (const gran::GranException& error)
        {
            badPath = error.getCode() == gran::ErrorCode::MARKUP;
        }
        TEST_ASSERT(badPath);
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

    // Последний столбец и строка сохраняют дробные площади при обоих направлениях обхода.
    // The last column and row retain their fractional coverage in both winding directions.
    void testRasterEdgeCoverage()
    {
        const gran::Vec2 corners[] = {{1.25f, 1}, {2.5f, 1}, {2.5f, 2.5f}, {1.25f, 2.5f}};
        gran::text::CoverageAccumulator forward(3, 3);
        gran::text::CoverageAccumulator reverse(3, 3);
        std::vector<std::uint8_t> coverage;
        std::vector<std::uint8_t> reversed;

        for (std::size_t index = 0; index < 4; ++index)
        {
            const std::size_t next = (index + 1) % 4;
            forward.addLine(corners[index], corners[next]);
            reverse.addLine(corners[next], corners[index]);
        }
        forward.resolve(coverage);
        reverse.resolve(reversed);
        TEST_ASSERT((coverage == std::vector<std::uint8_t>{0, 0, 0, 0, 191, 128, 0, 96, 64}));
        TEST_ASSERT(reversed == coverage);
    }

    // Правая граница растра шириной в один пиксель не влияет на следующую строку.
    // The right edge of a one-pixel-wide raster does not affect its next row.
    void testSingleColumnRaster()
    {
        gran::text::CoverageAccumulator accumulator(1, 2);
        std::vector<std::uint8_t> coverage;

        accumulator.addLine({0.5f, 1}, {0.5f, 0});
        accumulator.addLine({1, 0}, {1, 1});
        accumulator.resolve(coverage);
        TEST_ASSERT((coverage == std::vector<std::uint8_t>{128, 0}));
    }

    // Открытые поля Path допускают повреждённый ввод в обход разбора SVG.
    // Path's public fields allow malformed input to bypass SVG parsing.
    void testMalformedRasterPath()
    {
        using gran::vector::Path;
        using gran::vector::Verb;
        gran::vector::Shape shape;
        gran::text::GlyphBitmap bitmap;

        shape.paths = {Path{{Verb::MOVE, Verb::CUBIC}, {{1, 1}, {2, 2}}}};
        TEST_ASSERT(!gran::vector::rasterizeShape(shape, 24, 24, bitmap));
        shape.paths = {Path{{Verb::LINE}, {{2, 2}}}};
        TEST_ASSERT(!gran::vector::rasterizeShape(shape, 24, 24, bitmap));
        shape.paths = {Path{{Verb::CUBIC}, {}}};
        TEST_ASSERT(!gran::vector::rasterizeShape(shape, 24, 24, bitmap));
    }

    // После CLOSE следующая команда использует начало замкнутого подконтура.
    // After CLOSE, the next command starts from the closed subpath's origin.
    void testRasterPathAfterClose()
    {
        gran::vector::Shape shape;
        gran::text::GlyphBitmap bitmap;

        shape.paths.emplace_back();
        shape.paths.push_back(gran::vector::parsePathData("M1 1 Z Z H5 V5 H1 Z"));
        TEST_ASSERT(gran::vector::rasterizeShape(shape, 24, 24, bitmap));
        TEST_ASSERT(coverageAt(bitmap, 2, 2) == 255);
    }
}

namespace suites
{
    void registerVectorShapeTests(test::TestRunner& runner)
    {
        runner.add("vector: valid viewBox wins and missing dimensions default independently", testSvgDimensions);
        runner.add("vector: rejects invalid explicit SVG dimensions and caches the failure", testInvalidSvgDimensions);
        runner.add("vector: rectangle radii and a zero-sized viewBox", testSvgRectRadii);
        runner.add("vector: parsePathData", testParsePathData);
        runner.add("vector: extreme arc radii yield finite path points", testArcExtremeRadii);
        runner.add("vector: shallow relative arcs keep their control points along the chord", testShallowRelativeArc);
        runner.add("vector: rotates extreme ellipses and resets the cubic reflection after an arc", testArcRotationAndFollowingCurve);
        runner.add("vector: rejects an arc with unrepresentable control points", testArcOutOfRange);
        runner.add("vector: parseViewBox", testParseViewBox);
        runner.add("vector: rasterizeShape", testRasterizeShape);
        runner.add("vector: evenOdd", testEvenOdd);
        runner.add("raster: reversed coverage at the right and bottom edges", testRasterEdgeCoverage);
        runner.add("raster: one column with an exact right edge", testSingleColumnRaster);
        runner.add("vector: malformed raster path", testMalformedRasterPath);
        runner.add("vector: raster path after close", testRasterPathAfterClose);
    }
}
