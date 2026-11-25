#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/app/internal/web_slots.hpp"
#include "gran/app/internal/app_state.hpp"
#include "gran/platform/internal/web_message_limits.hpp"
#include <string_view>
#include <stdexcept>

namespace
{
    struct RecordingView final : gran::platform::WebView
    {
        std::string received;
        bool interrupt = true;
        void navigate(const std::string&) override {}
        void loadHtml(const std::string&) override {}
        void setBounds(const gran::Rect&, const gran::Rect&) override {}
        void setVisible(bool) override {}
        void postMessage(const std::string& message) override
        {
            if (message == "B" && interrupt)
            {
                interrupt = false;
                throw std::runtime_error("interrupted");
            }
            received += message;
        }
    };

    void testWebQueueRetry()
    {
        gran::Application::State state;
        gran::app::AppWindow window(state);
        gran::ui::Node page;
        gran::app::WebSlot slot(window, page);
        slot.view = std::make_unique<RecordingView>();
        slot.outbox = {"A", "B", "C"};
        TEST_ASSERT_THROWS(gran::app::flushWebOutbox(slot), std::runtime_error);
        TEST_ASSERT(slot.outbox.size() == 2 && slot.outbox.front() == "B");
        gran::app::flushWebOutbox(slot);
        TEST_ASSERT(slot.outbox.empty() && static_cast<RecordingView&>(*slot.view).received == "ABC");
    }

    void testWebQueueLimits()
    {
        gran::Application app;
        app.loadString(R"(<Window><WebView id="page"/></Window>)");
        auto& page = *app.find("page");
        gran::Application::State state;
        gran::app::AppWindow window(state);
        gran::app::WebSlot slot(window, page);
        const std::string oversized(gran::platform::WEB_TEXT_LIMIT + 1, 'x');
        TEST_ASSERT_THROWS(app.postWebMessage(page, oversized), gran::GranException);
        slot.webMessage(oversized);
        TEST_ASSERT(state.webInbox.empty());
        for (std::size_t i = 0; i < gran::platform::WEB_QUEUE_LIMIT; ++i)
        {
            app.postWebMessage(page, "pending");
            slot.webMessage("accepted");
        }
        TEST_ASSERT_THROWS(app.postWebMessage(page, "overflow"), gran::GranException);
        slot.webMessage("overflow");
        TEST_ASSERT(state.webInbox.size() == gran::platform::WEB_QUEUE_LIMIT);
        TEST_ASSERT(state.webInbox.back().text == "accepted");
    }

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
        runner.add("web view: bounded queues", testWebQueueLimits);
        runner.add("web view: retry pending messages", testWebQueueRetry);
        runner.add("web view: markup exception", testWebViewMarkupException);
    }
}
