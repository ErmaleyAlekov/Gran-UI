#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/style/values.hpp"
#include "gran/utils/geometry.hpp"
#include "gran/app/application.hpp"
#include "gran/markup/markup_loader.hpp"
#include <cmath>
#include <limits>
#include <string>

namespace
{
    void testInputBoundsHugeNumbers()
    {
        std::string many(400, '9');
        TEST_ASSERT_THROWS(gran::style::parseNumber(many), gran::GranException); // Бесконечность
        TEST_ASSERT_THROWS(gran::style::parseNumber(many + "." + many), gran::GranException); // NaN
        TEST_ASSERT_THROWS(gran::style::parseNumber("1e40"), gran::GranException); // Большое число
        TEST_ASSERT_THROWS(gran::style::parseNumber("1e999"), gran::GranException); // Очень большое число
        TEST_ASSERT_NEAR(gran::style::parseNumber("3e38"), 3e38f, 1e33f); // Крайнее допустимое число
    }

    void testInputBoundsIntegers()
    {
        TEST_ASSERT(gran::style::parseInteger("12", 1, 100) == 12); // В пределах диапазона
        TEST_ASSERT(gran::style::parseInteger("-5", -10, 10) == -5); // В пределах диапазона
        TEST_ASSERT_THROWS(gran::style::parseInteger("0", 1, 100), gran::GranException); // Недопустимое значение
        TEST_ASSERT_THROWS(gran::style::parseInteger("101", 1, 100), gran::GranException); // Недопустимое значение
        TEST_ASSERT_THROWS(gran::style::parseInteger("1.5", 1, 100), gran::GranException); // Недопустимое значение
        TEST_ASSERT_THROWS(gran::style::parseInteger("1e30", 1, 100), gran::GranException); // Недопустимое значение
        TEST_ASSERT_THROWS(gran::style::parseInteger("-3", 1, 100), gran::GranException); // Недопустимое значение
        TEST_ASSERT_THROWS(gran::style::parseInteger("abc", 1, 100), gran::GranException); // Недопустимое значение
        TEST_ASSERT_THROWS(gran::style::parseEasing("steps(1e30)"), gran::GranException); // Недопустимое значение
        TEST_ASSERT_THROWS(gran::style::parseEasing("steps(0)"), gran::GranException); // Недопустимое значение
        TEST_ASSERT(gran::style::parseEasing("steps(4)").steps == 4); // Допустимое значение
    }

    void testInputBoundsMarkupCounts()
    {
        TEST_ASSERT_THROWS(gran::markup::loadMarkupString(R"(<Window><TextArea rows="99999999999999999999"/></Window>)", gran::markup::LoadOptions{}), gran::GranException); // Недопустимое значение
        TEST_ASSERT_THROWS(gran::markup::loadMarkupString(R"(<Window><TextBox maxLength="99999999999999999999"/></Window>)", gran::markup::LoadOptions{}), gran::GranException); // Недопустимое значение
        auto doc = gran::markup::loadMarkupString(R"(<Window><TextArea id="t" rows="3"/></Window>)", gran::markup::LoadOptions{}); // Допустимое значение
        TEST_ASSERT(doc.root != nullptr);
    }

    void testInputBoundsVariableNumber()
    {
        gran::Application app;
        TEST_ASSERT_THROWS(app.setVariableNumber("x", std::numeric_limits<double>::infinity()), gran::GranException); // Бесконечность
        TEST_ASSERT_THROWS(app.setVariableNumber("x", std::nan("")), gran::GranException); // NaN
        app.setVariableNumber("x", 2.5); // Допустимое значение
        TEST_ASSERT(app.variable("x").has_value());
    }
}

namespace suites
{
    void registerInputBoundsTests(test::TestRunner& runner)
    {
        runner.add("input bounds: huge numbers", testInputBoundsHugeNumbers);
        runner.add("input bounds: integers", testInputBoundsIntegers);
        runner.add("input bounds: markup counts", testInputBoundsMarkupCounts);
        runner.add("input bounds: variable number", testInputBoundsVariableNumber);
    }
}
