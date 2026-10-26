#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/style/values.hpp"
#include "gran/utils/geometry.hpp"

namespace
{
    void testParseNumber()
    {
        TEST_ASSERT_NEAR(gran::style::parseNumber("-12.5"), -12.5f);
        TEST_ASSERT_NEAR(gran::style::parseNumber("1e2"), 100.0f);
        TEST_ASSERT_NEAR(gran::style::parseNumber(".5"), 0.5f);
        TEST_ASSERT_THROWS(gran::style::parseNumber("12px"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseNumber("abc"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseNumber(""), gran::GranException);
    }

    void testParseLength()
    {
        gran::style::Length length = gran::style::parseLength("auto");
        TEST_ASSERT(length.unit == gran::style::LengthUnit::AUTO);

        length = gran::style::parseLength("120");
        TEST_ASSERT(length.unit == gran::style::LengthUnit::PIXELS);
        TEST_ASSERT_NEAR(length.value, 120.0f);

        length = gran::style::parseLength("120px");
        TEST_ASSERT(length.unit == gran::style::LengthUnit::PIXELS);
        TEST_ASSERT_NEAR(length.value, 120.0f);

        length = gran::style::parseLength("50%");
        TEST_ASSERT(length.unit == gran::style::LengthUnit::PERCENT);
        TEST_ASSERT_NEAR(length.value, 50.0f);

        TEST_ASSERT_THROWS(gran::style::parseLength("12em"), gran::GranException);
    }

    void testParseDuration()
    {
        TEST_ASSERT_NEAR(gran::style::parseDuration("200ms"), 0.2f);
        TEST_ASSERT_NEAR(gran::style::parseDuration("0.5s"), 0.5f);
        TEST_ASSERT_THROWS(gran::style::parseDuration("-1s"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseDuration("5"), gran::GranException);
    }

    void testParseAngle()
    {
        TEST_ASSERT_NEAR(gran::style::parseAngle("180deg"), 3.14159f);
        TEST_ASSERT_NEAR(gran::style::parseAngle("0.5turn"), 3.14159f);
        TEST_ASSERT_NEAR(gran::style::parseAngle("90"), 1.5708f);
    }

    void testParseFraction()
    {
        TEST_ASSERT_NEAR(gran::style::parseFraction("50%"), 0.5f);
    }

    void testParseInsets()
    {
        gran::Insets insets = gran::style::parseInsets("8 16");
        TEST_ASSERT_NEAR(insets.top, 8.0f);
        TEST_ASSERT_NEAR(insets.right, 16.0f);
        TEST_ASSERT_NEAR(insets.bottom, 8.0f);
        TEST_ASSERT_NEAR(insets.left, 16.0f);

        insets = gran::style::parseInsets("1,2,3");
        TEST_ASSERT_NEAR(insets.left, 2.0f);

        TEST_ASSERT_THROWS(gran::style::parseInsets("1 2 3 4 5"), gran::GranException);
    }

    void testParseRadii()
    {
        gran::CornerRadii radii = gran::style::parseRadii("4");
        TEST_ASSERT_NEAR(radii.topLeft, 4.0f);
        TEST_ASSERT_NEAR(radii.topRight, 4.0f);
        TEST_ASSERT_NEAR(radii.bottomRight, 4.0f);
        TEST_ASSERT_NEAR(radii.bottomLeft, 4.0f);

        TEST_ASSERT_THROWS(gran::style::parseRadii("-1"), gran::GranException);
    }

    void testParseKeyword()
    {
        TEST_ASSERT(gran::style::parseBoolean("true"));
        TEST_ASSERT(!gran::style::parseBoolean("0"));
        TEST_ASSERT_THROWS(gran::style::parseBoolean("yes"), gran::GranException);
    }
}

namespace suites
{
    void registerStyleNumberTests(test::TestRunner& runner)
    {
        runner.add("style: parseNumber", testParseNumber);
        runner.add("style: parseLength", testParseLength);
        runner.add("style: parseDuration", testParseDuration);
        runner.add("style: parseAngle", testParseAngle);
        runner.add("style: parseFraction", testParseFraction);
        runner.add("style: parseInsets", testParseInsets);
        runner.add("style: parseRadii", testParseRadii);
        runner.add("style: parseKeyword", testParseKeyword);
    }
}
