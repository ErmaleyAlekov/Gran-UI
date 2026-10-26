#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/utils/color.hpp"

namespace
{
    // Проверка парсинга цвета из шестнадцатеричного кода
    // Test parsing color from hexadecimal code
    void testParseColorHex()
    {
        auto color = gran::style::parseColor("#fff");
        TEST_ASSERT_NEAR(color.r, 1.0f);
        TEST_ASSERT_NEAR(color.g, 1.0f);
        TEST_ASSERT_NEAR(color.b, 1.0f);
        TEST_ASSERT_NEAR(color.a, 1.0f);

        color = gran::style::parseColor("#ff000080");
        TEST_ASSERT_NEAR(color.r, 1.0f);
        TEST_ASSERT_NEAR(color.a, 0.502f);

        color = gran::style::parseColor("#0f08");
        TEST_ASSERT_NEAR(color.g, 1.0f);
        TEST_ASSERT_NEAR(color.a, 0.533f);
    }

    // Проверка парсинга цвета из функции rgb
    // Test parsing color from rgb function
    void testParseColorRgb()
    {
        auto color = gran::style::parseColor("rgb(255, 0, 0)");
        TEST_ASSERT_NEAR(color.r, 1.0f);

        color = gran::style::parseColor("rgba(0,0,255,0.5)");
        TEST_ASSERT_NEAR(color.b, 1.0f);
        TEST_ASSERT_NEAR(color.a, 0.5f);

        color = gran::style::parseColor("rgb(0 128 0 / 50%)");
        TEST_ASSERT_NEAR(color.g, 0.502f);
        TEST_ASSERT_NEAR(color.a, 0.5f);
    }

    // Проверка парсинга цвета из функции hsl
    // Test parsing color from hsl function
    void testParseColorHsl()
    {
        auto color = gran::style::parseColor("hsl(120, 100%, 50%)");
        TEST_ASSERT_NEAR(color.g, 1.0f);
        TEST_ASSERT_NEAR(color.r, 0.0f);
        TEST_ASSERT_NEAR(color.b, 0.0f);
    }

    // Проверка парсинга цвета по имени
    // Test parsing color by name
    void testParseColorName()
    {
        auto color = gran::style::parseColor("RebeccaPurple");
        TEST_ASSERT_NEAR(color.r, 0.4f);
        TEST_ASSERT_NEAR(color.g, 0.2f);
        TEST_ASSERT_NEAR(color.b, 0.6f);

        color = gran::style::parseColor("transparent");
        TEST_ASSERT_NEAR(color.a, 0.0f);
    }

    // Проверка обработки ошибок при парсинге цвета
    // Test error handling during color parsing
    void testParseColorErrors()
    {
        TEST_ASSERT_THROWS(gran::style::parseColor("#ff"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseColor("#ggg"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseColor("rgb(1,2)"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseColor("notacolor"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseColor("rgb(1,2,3"), gran::GranException);
    }
}

namespace suites
{
    // Регистрация набора тестов для парсинга цвета
    // The suite registration for color parsing tests
    void registerStyleColorTests(test::TestRunner& runner)
    {
        runner.add("style: парсинг цвета из шестнадцатеричного кода", testParseColorHex);
        runner.add("style: парсинг цвета из функции rgb", testParseColorRgb);
        runner.add("style: парсинг цвета из функции hsl", testParseColorHsl);
        runner.add("style: парсинг цвета по имени", testParseColorName);
        runner.add("style: обработка ошибок при парсинге цвета", testParseColorErrors);
    }
}
