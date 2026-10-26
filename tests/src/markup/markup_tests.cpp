#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"
#include "gran/core/gran_exception.hpp"

namespace
{
    void testMarkupLoading()
    {
        auto document = support::layoutMarkup(R"(<Window><Stack><Text id="t">  Привет
    мир </Text></Stack></Window>)", 400, 300);
        TEST_ASSERT(document.nodeCount == 2);
        const auto& textNode = support::nodeById(document, "t");
        TEST_ASSERT(textNode.content.text == "Привет мир");

        TEST_ASSERT_THROWS(support::layoutMarkup(R"(<Window><Box minWidth="300" maxWidth="20"/></Window>)", 400, 300), gran::MarkupException);
        TEST_ASSERT_THROWS(support::layoutMarkup(R"(<Window><Divv/></Window>)", 400, 300), gran::MarkupException);

        try {
            support::layoutMarkup(R"(<Window><Box minWidth="300" maxWidth="20"/></Window>)", 400, 300);
        } catch (const gran::MarkupException& e) {
            TEST_ASSERT(e.getLine() == 1);
        }

        try {
            support::layoutMarkup(R"(<Window><Divv/></Window>)", 400, 300);
        } catch (const gran::MarkupException& e) {
            TEST_ASSERT(e.getLine() == 1);
        }

        document = support::layoutMarkup(R"(<Window><Resources><Theme><Token name="a" value="#ff0000"/></Theme></Resources><Box id="b" background="$a"/></Window>)", 400, 300);
        const auto& boxNode = support::nodeById(document, "b");
        TEST_ASSERT(boxNode.computed.background.kind == gran::style::PaintKind::COLOR);
        TEST_ASSERT_NEAR(boxNode.computed.background.color.r, 1.0f, 0.001f);
        TEST_ASSERT_NEAR(boxNode.computed.background.color.g, 0.0f, 0.001f);

        TEST_ASSERT_THROWS(support::layoutMarkup(R"(<Window><Resources><Theme><Token name="a" value="#ff0000"/></Theme></Resources><Box id="b" background="$zz"/></Window>)", 400, 300), gran::MarkupException);

        document = support::layoutMarkup(R"(<Window><Resources><Style name="base" padding="7"/><Style name="child" basedOn="base" borderWidth="2"/></Resources><Box id="x" style="child"/></Window>)", 400, 300);
        const auto& boxNode2 = support::nodeById(document, "x");
        TEST_ASSERT_NEAR(boxNode2.computed.padding.top, 7.0f, 0.001f);
        TEST_ASSERT_NEAR(boxNode2.computed.borderWidth.top, 2.0f, 0.001f);

        TEST_ASSERT_THROWS(support::layoutMarkup(R"(<Window><Resources><Style name="a" basedOn="b"/><Style name="b" basedOn="a"/></Resources><Box id="x"/></Window>)", 400, 300), gran::MarkupException);

        document = support::layoutMarkup(R"(<Window><Stack><Box id="x" background="#000"><Hover background="#fff"/></Box><Button id="b" text="OK"/><Slider id="s" min="0" max="10" value="4"/></Stack></Window>)", 400, 300);
        const auto& boxNode3 = support::nodeById(document, "x");
        TEST_ASSERT(boxNode3.declared.states[0].size() > 0);

        const auto& buttonNode = support::nodeById(document, "b");
        TEST_ASSERT_NEAR(buttonNode.computed.borderRadius.topLeft, 8.0f, 0.001f);

        const auto& sliderNode = support::nodeById(document, "s");
        TEST_ASSERT_NEAR(sliderNode.content.value, 4.0f, 0.001f);
    }
}

namespace suites
{
    void registerMarkupTests(test::TestRunner& runner)
    {
        runner.add("markup: loading and parsing", testMarkupLoading);
    }
}
