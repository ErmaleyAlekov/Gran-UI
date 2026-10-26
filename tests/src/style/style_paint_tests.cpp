#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/style/values.hpp"

namespace
{
    void testParsePaintNone()
    {
        gran::style::Paint paint = gran::style::parsePaint("none");
        TEST_ASSERT(paint.kind == gran::style::PaintKind::NONE);
    }

    void testParsePaintColor()
    {
        gran::style::Paint paint = gran::style::parsePaint("#000");
        TEST_ASSERT(paint.kind == gran::style::PaintKind::COLOR);
        TEST_ASSERT(paint.color.r == 0.0f);
        TEST_ASSERT(paint.color.g == 0.0f);
        TEST_ASSERT(paint.color.b == 0.0f);
        TEST_ASSERT(paint.color.a == 1.0f);
    }

    void testParsePaintLinearGradient()
    {
        gran::style::Paint paint = gran::style::parsePaint("linear-gradient(90deg, red, blue)");
        TEST_ASSERT(paint.kind == gran::style::PaintKind::GRADIENT);
        TEST_ASSERT_NEAR(paint.gradient.angle, 1.5708f, 0.0001f);
        TEST_ASSERT(paint.gradient.stops.size() == 2);
        TEST_ASSERT(paint.gradient.stops[0].position == 0.0f);
        TEST_ASSERT(paint.gradient.stops[1].position == 1.0f);
    }

    void testParseGradientLinear()
    {
        gran::style::Gradient gradient = gran::style::parseGradient("linear-gradient(to right, red, #0f0 30%, blue)");
        TEST_ASSERT_NEAR(gradient.angle, 1.5708f, 0.0001f);
        TEST_ASSERT(gradient.stops.size() == 3);
        TEST_ASSERT_NEAR(gradient.stops[1].position, 0.3f, 0.0001f);
    }

    void testParseGradientDefault()
    {
        gran::style::Gradient gradient = gran::style::parseGradient("linear-gradient(red, green, blue)");
        TEST_ASSERT_NEAR(gradient.angle, 3.14159f, 0.0001f);
        TEST_ASSERT(gradient.stops.size() == 3);
        TEST_ASSERT_NEAR(gradient.stops[1].position, 0.5f, 0.0001f);
    }

    void testParseGradientRadial()
    {
        gran::style::Gradient gradient = gran::style::parseGradient("radial-gradient(circle at 25% 75%, white, black)");
        TEST_ASSERT(gradient.kind == gran::style::GradientKind::RADIAL);
        TEST_ASSERT(gradient.circle);
        TEST_ASSERT_NEAR(gradient.center.x, 0.25f, 0.0001f);
        TEST_ASSERT_NEAR(gradient.center.y, 0.75f, 0.0001f);
    }

    void testParseGradientConic()
    {
        gran::style::Gradient gradient = gran::style::parseGradient("conic-gradient(from 90deg, red, blue)");
        TEST_ASSERT(gradient.kind == gran::style::GradientKind::CONIC);
        TEST_ASSERT_NEAR(gradient.angle, 1.5708f, 0.0001f);
    }

    void testParseGradientOrder()
    {
        gran::style::Gradient gradient = gran::style::parseGradient("linear-gradient(red 50%, blue 20%)");
        TEST_ASSERT(gradient.stops.size() == 2);
        TEST_ASSERT_NEAR(gradient.stops[1].position, 0.5f, 0.0001f);
    }

    void testParseGradientErrors()
    {
        TEST_ASSERT_THROWS(gran::style::parseGradient("linear-gradient(red)"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseGradient("foo-gradient(red, blue)"), gran::GranException);
    }

    void testParseShadows()
    {
        std::vector<gran::style::Shadow> shadows = gran::style::parseShadows("0 4 12 rgba(0,0,0,0.25)");
        TEST_ASSERT(shadows.size() == 1);
        TEST_ASSERT_NEAR(shadows[0].offsetY, 4.0f, 0.0001f);
        TEST_ASSERT_NEAR(shadows[0].blur, 12.0f, 0.0001f);
        TEST_ASSERT_NEAR(shadows[0].color.a, 0.25f, 0.0001f);
    }

    void testParseShadowsInset()
    {
        std::vector<gran::style::Shadow> shadows = gran::style::parseShadows("inset 0 1 0 #fff, 0 2 8 2 black");
        TEST_ASSERT(shadows.size() == 2);
        TEST_ASSERT(shadows[0].inset);
        TEST_ASSERT_NEAR(shadows[1].spread, 2.0f, 0.0001f);
    }

    void testParseShadowsNone()
    {
        std::vector<gran::style::Shadow> shadows = gran::style::parseShadows("none");
        TEST_ASSERT(shadows.empty());
    }

    void testParseShadowsErrors()
    {
        TEST_ASSERT_THROWS(gran::style::parseShadows("0 #000"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseShadows("0 0 -2 #000"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseShadows("0 0 0"), gran::GranException);
    }
}

namespace suites
{
    void registerStylePaintTests(test::TestRunner& runner)
    {
        runner.add("style: parsePaint none", testParsePaintNone);
        runner.add("style: parsePaint color", testParsePaintColor);
        runner.add("style: parsePaint linear gradient", testParsePaintLinearGradient);
        runner.add("style: parseGradient linear", testParseGradientLinear);
        runner.add("style: parseGradient default", testParseGradientDefault);
        runner.add("style: parseGradient radial", testParseGradientRadial);
        runner.add("style: parseGradient conic", testParseGradientConic);
        runner.add("style: parseGradient order", testParseGradientOrder);
        runner.add("style: parseGradient errors", testParseGradientErrors);
        runner.add("style: parseShadows", testParseShadows);
        runner.add("style: parseShadows inset", testParseShadowsInset);
        runner.add("style: parseShadows none", testParseShadowsNone);
        runner.add("style: parseShadows errors", testParseShadowsErrors);
    }
}
