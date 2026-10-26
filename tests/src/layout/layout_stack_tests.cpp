#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"

namespace
{
    void testStackLayoutVertical()
    {
        const char* markup = R"(<Window><Stack id="s" padding="10" spacing="5"><Box id="a" height="20"/><Box id="b" height="30"/></Stack></Window>)";
        auto document = support::layoutMarkup(markup, 200, 100);

        const auto& stack = support::nodeById(document, "s");
        TEST_ASSERT_NEAR(stack.layout.frame.x, 0, 0.001f);
        TEST_ASSERT_NEAR(stack.layout.frame.y, 0, 0.001f);
        TEST_ASSERT_NEAR(stack.layout.frame.width, 200, 0.001f);
        TEST_ASSERT_NEAR(stack.layout.frame.height, 100, 0.001f);

        const auto& a = support::nodeById(document, "a");
        TEST_ASSERT_NEAR(a.layout.frame.x, 10, 0.001f);
        TEST_ASSERT_NEAR(a.layout.frame.y, 10, 0.001f);
        TEST_ASSERT_NEAR(a.layout.frame.width, 180, 0.001f);
        TEST_ASSERT_NEAR(a.layout.frame.height, 20, 0.001f);

        const auto& b = support::nodeById(document, "b");
        TEST_ASSERT_NEAR(b.layout.frame.x, 10, 0.001f);
        TEST_ASSERT_NEAR(b.layout.frame.y, 35, 0.001f);
        TEST_ASSERT_NEAR(b.layout.frame.width, 180, 0.001f);
        TEST_ASSERT_NEAR(b.layout.frame.height, 30, 0.001f);
    }

    void testStackLayoutHorizontal()
    {
        const char* markup1 = R"(<Window><Stack orientation="horizontal"><Box id="a" width="50"/><Box id="b" grow="1"/><Box id="c" grow="3"/></Stack></Window>)";
        auto document1 = support::layoutMarkup(markup1, 250, 50);

        const auto& a1 = support::nodeById(document1, "a");
        TEST_ASSERT_NEAR(a1.layout.frame.x, 0, 0.001f);
        TEST_ASSERT_NEAR(a1.layout.frame.width, 50, 0.001f);

        const auto& b1 = support::nodeById(document1, "b");
        TEST_ASSERT_NEAR(b1.layout.frame.x, 50, 0.001f);
        TEST_ASSERT_NEAR(b1.layout.frame.width, 50, 0.001f);

        const auto& c1 = support::nodeById(document1, "c");
        TEST_ASSERT_NEAR(c1.layout.frame.x, 100, 0.001f);
        TEST_ASSERT_NEAR(c1.layout.frame.width, 150, 0.001f);

        const char* markup2 = R"(<Window><Stack orientation="horizontal" justify="center"><Box id="a" width="40" height="10"/><Box id="b" width="60" height="10"/></Stack></Window>)";
        auto document2 = support::layoutMarkup(markup2, 200, 50);

        const auto& a2 = support::nodeById(document2, "a");
        TEST_ASSERT_NEAR(a2.layout.frame.x, 50, 0.001f);

        const auto& b2 = support::nodeById(document2, "b");
        TEST_ASSERT_NEAR(b2.layout.frame.x, 90, 0.001f);

        const char* markup3 = R"(<Window><Stack orientation="horizontal" justify="space-between"><Box id="a" width="40" height="10"/><Box id="b" width="60" height="10"/></Stack></Window>)";
        auto document3 = support::layoutMarkup(markup3, 200, 50);

        const auto& a3 = support::nodeById(document3, "a");
        TEST_ASSERT_NEAR(a3.layout.frame.x, 0, 0.001f);

        const auto& b3 = support::nodeById(document3, "b");
        TEST_ASSERT_NEAR(b3.layout.frame.x, 140, 0.001f);
    }

    void testStackLayoutCenter()
    {
        const char* markup1 = R"(<Window><Stack align="center"><Box id="a" width="40" height="10"/></Stack></Window>)";
        auto document1 = support::layoutMarkup(markup1, 200, 100);

        const auto& a1 = support::nodeById(document1, "a");
        TEST_ASSERT_NEAR(a1.layout.frame.x, 80, 0.001f);
        TEST_ASSERT_NEAR(a1.layout.frame.y, 0, 0.001f);

        const char* markup2 = R"(<Window><Stack orientation="horizontal" width="100"><Text id="t1">aaaaaaaaaa</Text><Text id="t2">aaaaaaaaaa</Text></Stack></Window>)";
        auto document2 = support::layoutMarkup(markup2, 300, 50);

        const auto& t1 = support::nodeById(document2, "t1");
        TEST_ASSERT_NEAR(t1.layout.frame.x, 0, 0.001f);
        TEST_ASSERT_NEAR(t1.layout.frame.width, 50, 0.001f);

        const auto& t2 = support::nodeById(document2, "t2");
        TEST_ASSERT_NEAR(t2.layout.frame.x, 50, 0.001f);
        TEST_ASSERT_NEAR(t2.layout.frame.width, 50, 0.001f);
    }

    void testStackLayoutAdvanced()
    {
        const char* markup1 = R"(<Window><Stack orientation="horizontal"><Box id="a" grow="1" maxWidth="60"/><Box id="b" grow="1"/></Stack></Window>)";
        auto document1 = support::layoutMarkup(markup1, 200, 50);

        const auto& a1 = support::nodeById(document1, "a");
        TEST_ASSERT_NEAR(a1.layout.frame.width, 60, 0.001f);

        const auto& b1 = support::nodeById(document1, "b");
        TEST_ASSERT_NEAR(b1.layout.frame.x, 60, 0.001f);
        TEST_ASSERT_NEAR(b1.layout.frame.width, 140, 0.001f);

        const char* markup2 = R"(<Window><Stack><Box id="a" height="10" margin="5 10"/></Stack></Window>)";
        auto document2 = support::layoutMarkup(markup2, 200, 100);

        const auto& a2 = support::nodeById(document2, "a");
        TEST_ASSERT_NEAR(a2.layout.frame.x, 10, 0.001f);
        TEST_ASSERT_NEAR(a2.layout.frame.y, 5, 0.001f);
        TEST_ASSERT_NEAR(a2.layout.frame.width, 180, 0.001f);
        TEST_ASSERT_NEAR(a2.layout.frame.height, 10, 0.001f);

        const char* markup3 = R"(<Window><Stack spacing="10"><Box id="a" height="10" visible="false"/><Box id="b" height="10"/></Stack></Window>)";
        auto document3 = support::layoutMarkup(markup3, 200, 100);

        const auto& b3 = support::nodeById(document3, "b");
        TEST_ASSERT_NEAR(b3.layout.frame.y, 0, 0.001f);

        const char* markup4 = R"(<Window><Stack orientation="horizontal" wrap="true" spacing="10"><Box id="a" width="80" height="20"/><Box id="b" width="80" height="20"/><Box id="c" width="80" height="20"/></Stack></Window>)";
        auto document4 = support::layoutMarkup(markup4, 200, 100);

        const auto& a4 = support::nodeById(document4, "a");
        TEST_ASSERT_NEAR(a4.layout.frame.x, 0, 0.001f);
        TEST_ASSERT_NEAR(a4.layout.frame.y, 0, 0.001f);

        const auto& b4 = support::nodeById(document4, "b");
        TEST_ASSERT_NEAR(b4.layout.frame.x, 90, 0.001f);
        TEST_ASSERT_NEAR(b4.layout.frame.y, 0, 0.001f);

        const auto& c4 = support::nodeById(document4, "c");
        TEST_ASSERT_NEAR(c4.layout.frame.x, 0, 0.001f);
        TEST_ASSERT_NEAR(c4.layout.frame.y, 30, 0.001f);
    }
}

namespace suites
{
    void registerLayoutStackTests(test::TestRunner& runner)
    {
        runner.add("layout: stack vertical", testStackLayoutVertical);
        runner.add("layout: stack horizontal", testStackLayoutHorizontal);
        runner.add("layout: stack center", testStackLayoutCenter);
        runner.add("layout: stack advanced", testStackLayoutAdvanced);
    }
}
