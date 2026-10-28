#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/app/internal/web_slots.hpp"
#include <string_view>

namespace
{
    void testWebSourceUrl()
    {
        TEST_ASSERT(gran::app::webSourceUrl("https://example.org/a?b=1", "C:/web") == "https://example.org/a?b=1");
        TEST_ASSERT(gran::app::webSourceUrl("about:blank", "C:/web") == "about:blank");
        TEST_ASSERT(gran::app::webSourceUrl("data:text/html,x", "C:/web") == "data:text/html,x");
#ifdef _WIN32
        TEST_ASSERT(gran::app::webSourceUrl("page.html", "C:/web") == "file:///C:/web/page.html");
        TEST_ASSERT(gran::app::webSourceUrl("my page#1.html", "C:/web") == "file:///C:/web/my%20page%231.html");
        TEST_ASSERT(gran::app::webSourceUrl("C:/abs/x.html", "C:/web") == "file:///C:/abs/x.html");
#else
        TEST_ASSERT(gran::app::webSourceUrl("page.html", "/web") == "file:///web/page.html");
        TEST_ASSERT(gran::app::webSourceUrl("my page#1.html", "/web") == "file:///web/my%20page%231.html");
        TEST_ASSERT(gran::app::webSourceUrl("/abs/x.html", "/web") == "file:///abs/x.html");
#endif
    }

    void testWebViewBehavior()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack><WebView id="w" width="100" height="80" source="https://example.org"/><Text id="t">x</Text></Stack></Window>)");

        auto* w = app.find("w");
        TEST_ASSERT(w != nullptr);
        TEST_ASSERT(w->kind == gran::ui::ElementKind::WEB_VIEW);
        TEST_ASSERT(w->content.source == "https://example.org");

        app.navigate(*w, "about:blank");
        TEST_ASSERT(w->content.source == "about:blank");

        app.loadHtml(*w, "<p>hi</p>");
        TEST_ASSERT(w->content.source.empty());
        TEST_ASSERT(w->content.text == "<p>hi</p>");

        app.postWebMessage(*w, "m");

        auto* t = app.find("t");
        TEST_ASSERT_THROWS(app.postWebMessage(*t, "m"), gran::GranException);
        TEST_ASSERT_THROWS(app.navigate(*t, "about:blank"), gran::GranException);
    }

    void testWebViewMarkupException()
    {
        gran::Application app;
        TEST_ASSERT_THROWS(app.loadString(R"(<Window><WebView source="a.html">текст</WebView></Window>)"), gran::MarkupException);
    }
}

namespace suites
{
    void registerWebViewTests(test::TestRunner& runner)
    {
        runner.add("web source url: basic functionality", testWebSourceUrl);
        runner.add("web view: basic behavior", testWebViewBehavior);
        runner.add("web view: markup exception", testWebViewMarkupException);
    }
}
