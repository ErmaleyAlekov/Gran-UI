#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/style/values.hpp"

namespace
{
    // Тесты для parseEasing
    void testParseEasing()
    {
        auto easing = gran::style::parseEasing("ease-in-out");
        TEST_ASSERT(easing.kind == gran::style::EasingKind::CUBIC_BEZIER);
        TEST_ASSERT_NEAR(easing.x1, 0.42f, 0.01f);
        TEST_ASSERT_NEAR(easing.x2, 0.58f, 0.01f);

        easing = gran::style::parseEasing("linear");
        TEST_ASSERT(easing.kind == gran::style::EasingKind::LINEAR);

        easing = gran::style::parseEasing("cubic-bezier(0.2, 0.8, 0.2, 1)");
        TEST_ASSERT(easing.kind == gran::style::EasingKind::CUBIC_BEZIER);
        TEST_ASSERT_NEAR(easing.y1, 0.8f, 0.01f);

        easing = gran::style::parseEasing("steps(4, start)");
        TEST_ASSERT(easing.kind == gran::style::EasingKind::STEPS);
        TEST_ASSERT(easing.steps == 4);
        TEST_ASSERT(easing.jumpStart);

        TEST_ASSERT_THROWS(gran::style::parseEasing("cubic-bezier(2, 0, 0, 1)"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseEasing("steps(0)"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseEasing("bounce"), gran::GranException);
    }

    // Тесты для parseTransitions
    void testParseTransitions()
    {
        auto transitions = gran::style::parseTransitions("background 0.2s ease, opacity 150ms linear 50ms");
        TEST_ASSERT(transitions.size() == 2);

        TEST_ASSERT(transitions[0].property == "background");
        TEST_ASSERT_NEAR(transitions[0].duration, 0.2f, 0.01f);
        TEST_ASSERT(transitions[0].easing.kind == gran::style::EasingKind::CUBIC_BEZIER);

        TEST_ASSERT(transitions[1].property == "opacity");
        TEST_ASSERT_NEAR(transitions[1].duration, 0.15f, 0.01f);
        TEST_ASSERT_NEAR(transitions[1].delay, 0.05f, 0.01f);
        TEST_ASSERT(transitions[1].easing.kind == gran::style::EasingKind::LINEAR);

        TEST_ASSERT_THROWS(gran::style::parseTransitions("background"), gran::GranException);
    }

    // Тесты для parseTransform
    void testParseTransform()
    {
        auto transform = gran::style::parseTransform("translate(10, -4) scale(1.5) rotate(90deg)");
        TEST_ASSERT(transform.size() == 3);

        TEST_ASSERT(transform[0].kind == gran::style::TransformKind::TRANSLATE);
        TEST_ASSERT_NEAR(transform[0].x, 10.0f, 0.01f);
        TEST_ASSERT_NEAR(transform[0].y, -4.0f, 0.01f);

        TEST_ASSERT(transform[1].kind == gran::style::TransformKind::SCALE);
        TEST_ASSERT_NEAR(transform[1].x, 1.5f, 0.01f);
        TEST_ASSERT_NEAR(transform[1].y, 1.5f, 0.01f);

        TEST_ASSERT(transform[2].kind == gran::style::TransformKind::ROTATE);
        TEST_ASSERT_NEAR(transform[2].x, 1.5708f, 0.01f);

        transform = gran::style::parseTransform("translateY(8)");
        TEST_ASSERT(transform.size() == 1);
        TEST_ASSERT(transform[0].kind == gran::style::TransformKind::TRANSLATE);
        TEST_ASSERT_NEAR(transform[0].y, 8.0f, 0.01f);

        transform = gran::style::parseTransform("scaleX(2)");
        TEST_ASSERT(transform.size() == 1);
        TEST_ASSERT(transform[0].kind == gran::style::TransformKind::SCALE);
        TEST_ASSERT_NEAR(transform[0].x, 2.0f, 0.01f);
        TEST_ASSERT_NEAR(transform[0].y, 1.0f, 0.01f);

        transform = gran::style::parseTransform("none");
        TEST_ASSERT(transform.empty());

        TEST_ASSERT_THROWS(gran::style::parseTransform("spin(3)"), gran::GranException);
        TEST_ASSERT_THROWS(gran::style::parseTransform("rotate(1, 2)"), gran::GranException);
    }
}

namespace suites
{
    // Регистрация набора / The suite registration
    void registerStyleMotionTests(test::TestRunner& runner)
    {
        runner.add("style: parseEasing", testParseEasing);
        runner.add("style: parseTransitions", testParseTransitions);
        runner.add("style: parseTransform", testParseTransform);
    }
}
