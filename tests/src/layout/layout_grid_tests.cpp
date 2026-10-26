#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"

namespace
{
    void testGridLayout()
    {
        const char* markup1 = R"(<Window><Grid rows="40, 1fr" columns="100, 1fr, 2fr"><Box id="a"/><Box id="b" gridRow="1" gridColumn="2"/><Box id="c" gridColumn="1" gridColumnSpan="2"/></Grid></Window>)";
        auto doc1 = support::layoutMarkup(markup1, 400, 300);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "a").layout.frame.x, 0.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "a").layout.frame.y, 0.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "a").layout.frame.width, 100.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "a").layout.frame.height, 40.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "b").layout.frame.x, 200.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "b").layout.frame.y, 40.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "b").layout.frame.width, 200.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "b").layout.frame.height, 260.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "c").layout.frame.x, 100.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "c").layout.frame.y, 0.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "c").layout.frame.width, 300.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc1, "c").layout.frame.height, 40.0f, 0.001f);

        const char* markup2 = R"(<Window><Grid rows="50, 50" columns="1fr, 1fr" rowSpacing="10" columnSpacing="20"><Box id="a"/><Box id="d" gridRow="1" gridColumn="1"/></Grid></Window>)";
        auto doc2 = support::layoutMarkup(markup2, 220, 200);
        TEST_ASSERT_NEAR(support::nodeById(doc2, "a").layout.frame.x, 0.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc2, "a").layout.frame.y, 0.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc2, "a").layout.frame.width, 100.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc2, "a").layout.frame.height, 50.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc2, "d").layout.frame.x, 120.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc2, "d").layout.frame.y, 60.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc2, "d").layout.frame.width, 100.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc2, "d").layout.frame.height, 50.0f, 0.001f);

        const char* markup3 = R"(<Window><Grid columns="auto, 1fr"><Text id="t">abcd</Text><Box id="b" gridColumn="1"/></Grid></Window>)";
        auto doc3 = support::layoutMarkup(markup3, 300, 100);
        TEST_ASSERT_NEAR(support::nodeById(doc3, "t").layout.frame.width, 28.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc3, "b").layout.frame.x, 28.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc3, "b").layout.frame.width, 272.0f, 0.001f);

        const char* markup4 = R"(<Window><Grid columns="minmax(150, 1fr), 1fr"><Box id="a"/><Box id="b" gridColumn="1"/></Grid></Window>)";
        auto doc4 = support::layoutMarkup(markup4, 200, 100);
        TEST_ASSERT_NEAR(support::nodeById(doc4, "a").layout.frame.width, 150.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc4, "b").layout.frame.x, 150.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc4, "b").layout.frame.width, 50.0f, 0.001f);

        const char* markup5 = R"(<Window><Grid columns="100" rows="100"><Box id="a" width="40" height="20" alignSelf="center"/></Grid></Window>)";
        auto doc5 = support::layoutMarkup(markup5, 300, 300);
        TEST_ASSERT_NEAR(support::nodeById(doc5, "a").layout.frame.x, 30.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc5, "a").layout.frame.y, 40.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc5, "a").layout.frame.width, 40.0f, 0.001f);
        TEST_ASSERT_NEAR(support::nodeById(doc5, "a").layout.frame.height, 20.0f, 0.001f);
    }
}

namespace suites
{
    void registerLayoutGridTests(test::TestRunner& runner)
    {
        runner.add("layout: grid", testGridLayout);
    }
}
