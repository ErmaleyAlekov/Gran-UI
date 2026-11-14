#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include "support/layout_helpers.hpp"
#include <cmath>
#include <string>

namespace
{
    class ActionSinkStub : public gran::input::ActionSink
    {
    public:
        void action(const std::string& name, gran::ui::Node&) override
        {
            lastAction = name;
            actionCount++;
        }

        std::string lastAction;
        int actionCount = 0;
    };

    class TextLocatorStub : public gran::input::TextLocator
    {
    public:
        std::size_t offsetAt(const gran::ui::Node&, gran::Vec2) override
        {
            return 0;
        }

        std::size_t lineEdge(const gran::ui::Node&, std::size_t offset, bool) override
        {
            return offset;
        }

        std::size_t lineMove(const gran::ui::Node&, std::size_t offset, int, float&) override
        {
            return offset;
        }

        int pageLines(const gran::ui::Node&) override
        {
            return 1;
        }
    };

    const char* MARKUP = R"(
        <Window>
            <Stack grow="1">
                <Splitter id="split" grow="1" position="200" minFirst="100" minSecond="150" collapse="first" onChange="changed">
                    <Box id="left"/>
                    <Box id="right"/>
                </Splitter>
            </Stack>
        </Window>
    )";

    void testLayout()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* split = gran::ui::findNode(*doc.root, "split");
        auto* handle = gran::ui::splitterHandle(*split);
        auto* left = split->children[0].get();
        auto* right = split->children[2].get();

        TEST_ASSERT(split->children.size() == 3);
        TEST_ASSERT(handle != nullptr);
        TEST_ASSERT_NEAR(left->layout.frame.width, 200.0f, 0.5f);
        TEST_ASSERT_NEAR(handle->layout.frame.x, left->layout.frame.x + 200.0f, 0.5f);
        TEST_ASSERT_NEAR(right->layout.frame.x, handle->layout.frame.x + handle->layout.frame.width, 0.5f);
        TEST_ASSERT_NEAR(right->layout.frame.x + right->layout.frame.width, 600.0f, 0.5f);
    }

    void testLimits()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* split = gran::ui::findNode(*doc.root, "split");
        auto* handle = gran::ui::splitterHandle(*split);
        auto* left = split->children[0].get();

        float h = handle->layout.frame.width;
        gran::input::setSplit(*split, 20);
        support::FixedMeasurer m;
        gran::layout::layoutTree(*doc.root, gran::Size{600, 400}, m);
        TEST_ASSERT_NEAR(left->layout.frame.width, 100.0f, 0.5f);

        gran::input::setSplit(*split, 1000);
        gran::layout::layoutTree(*doc.root, gran::Size{600, 400}, m);
        TEST_ASSERT_NEAR(left->layout.frame.width, 600.0f - h - 150.0f, 0.5f);
    }

    void testPercent()
    {
        const char* MARKUP_PERCENT = R"(
            <Window>
                <Stack grow="1">
                    <Splitter id="split" grow="1" position="25%" minFirst="100" minSecond="150" collapse="first" onChange="changed">
                        <Box id="left"/>
                        <Box id="right"/>
                    </Splitter>
                </Stack>
            </Window>
        )";

        auto doc = support::layoutMarkup(MARKUP_PERCENT, 600, 400);
        auto* split = gran::ui::findNode(*doc.root, "split");
        auto* handle = gran::ui::splitterHandle(*split);
        auto* left = split->children[0].get();

        float h = handle->layout.frame.width;
        TEST_ASSERT_NEAR(left->layout.frame.width, std::round((600.0f - h) * 0.25f), 0.5f);
    }

    void testKeysAndCollapse()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* split = gran::ui::findNode(*doc.root, "split");
        auto* handle = gran::ui::splitterHandle(*split);
        auto* left = split->children[0].get();

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);
        router.focus(handle);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::RIGHT;
        router.handle(event);
        support::FixedMeasurer m;
        gran::layout::layoutTree(*doc.root, gran::Size{600, 400}, m);
        TEST_ASSERT_NEAR(left->layout.frame.width, 210.0f, 0.5f);
        TEST_ASSERT(sink.lastAction == "changed");

        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(split->content.split.collapsed == true);
        gran::layout::layoutTree(*doc.root, gran::Size{600, 400}, m);
        TEST_ASSERT(!left->visible);
        TEST_ASSERT_NEAR(handle->layout.frame.x, 0.0f, 0.5f);

        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        gran::layout::layoutTree(*doc.root, gran::Size{600, 400}, m);
        TEST_ASSERT(left->visible);
        TEST_ASSERT_NEAR(left->layout.frame.width, 210.0f, 0.5f);
    }

    void testDrag()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* split = gran::ui::findNode(*doc.root, "split");
        auto* handle = gran::ui::splitterHandle(*split);
        auto* left = split->children[0].get();

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);
        router.focus(handle);

        gran::Vec2 c = {handle->layout.frame.x + handle->layout.frame.width / 2, handle->layout.frame.y + handle->layout.frame.height / 2};
        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        event.position = c;
        router.handle(event);
        TEST_ASSERT(router.cursor() == gran::platform::CursorShape::RESIZE_EW);

        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = c;
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {c.x + 50, c.y};
        router.handle(event);
        support::FixedMeasurer m;
        gran::layout::layoutTree(*doc.root, gran::Size{600, 400}, m);
        TEST_ASSERT_NEAR(left->layout.frame.width, 250.0f, 0.5f);

        event.kind = gran::platform::EventKind::MOUSE_UP;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {c.x + 50, c.y};
        router.handle(event);
        TEST_ASSERT(sink.lastAction == "changed");
    }
}

namespace suites
{
    void registerSplitterTests(test::TestRunner& runner)
    {
        runner.add("splitter: layout", testLayout);
        runner.add("splitter: limits", testLimits);
        runner.add("splitter: percent", testPercent);
        runner.add("splitter: keys and collapse", testKeysAndCollapse);
        runner.add("splitter: drag", testDrag);
    }
}
