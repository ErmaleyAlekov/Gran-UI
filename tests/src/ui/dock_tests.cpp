#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
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

    class WindowStub : public gran::platform::Window
    {
    public:
        bool pollEvent(gran::platform::Event&) override
        {
            return false;
        }

        void waitEvents(double) override
        {
        }

        void* nativeHandle() const override
        {
            return nullptr;
        }

        float scale() const override
        {
            return 1.0f;
        }

        gran::Size pixelSize() const override
        {
            return {0, 0};
        }

        void setCursor(gran::platform::CursorShape) override
        {
        }

        void show() override
        {
        }

        void setLiveResize(std::function<void()>) override
        {
        }

        std::string clipboardText() override
        {
            return clipboardText_;
        }

        void setClipboardText(const std::string& text) override
        {
            clipboardText_ = text;
        }

        void setAccessTree(gran::a11y::AccessTree*) override
        {
        }

        void accessFocusChanged() override
        {
        }

        void accessRefresh() override
        {
        }

        void wake() noexcept override
        {
        }

        std::string clipboardText_;
    };

    const char* MARKUP = R"(
        <Window>
            <Stack grow="1">
                <DockHost id="host" grow="1" onChange="changed">
                    <Text id="center">Центр</Text>
                    <DockPanel id="left1" title="Один" dock="left" size="150">
                        <Text>1</Text>
                    </DockPanel>
                    <DockPanel id="left2" title="Два" dock="left">
                        <Text>2</Text>
                    </DockPanel>
                    <DockPanel id="bottom" title="Низ" dock="bottom" size="100">
                        <Text>3</Text>
                    </DockPanel>
                </DockHost>
            </Stack>
        </Window>
    )";

    void testParts()
    {
        auto doc = support::layoutMarkup(MARKUP, 800, 600);
        auto* host = gran::ui::findNode(*doc.root, "host");
        auto* center = gran::ui::findNode(*doc.root, "center");

        TEST_ASSERT(gran::ui::dockStrip(*host, gran::ui::DockSide::LEFT) != nullptr);
        TEST_ASSERT(gran::ui::dockStrip(*host, gran::ui::DockSide::LEFT)->children.size() == 3);
        TEST_ASSERT(gran::ui::dockHandle(*host, gran::ui::DockSide::BOTTOM) != nullptr);
        TEST_ASSERT(gran::ui::dockCenter(*host) == center);
    }

    void testLayout()
    {
        auto doc = support::layoutMarkup(MARKUP, 800, 600);
        auto* host = gran::ui::findNode(*doc.root, "host");
        auto* left1 = gran::ui::findNode(*doc.root, "left1");
        auto* left2 = gran::ui::findNode(*doc.root, "left2");
        auto* bottom = gran::ui::findNode(*doc.root, "bottom");
        auto* center = gran::ui::findNode(*doc.root, "center");

        TEST_ASSERT(left1->visible);
        TEST_ASSERT_NEAR(left1->layout.frame.width, 150.0f, 0.5f);
        TEST_ASSERT(!left2->visible);
        TEST_ASSERT(left2->content.overflowed);
        TEST_ASSERT(bottom->visible);
        TEST_ASSERT_NEAR(bottom->layout.frame.y + bottom->layout.frame.height, 600.0f, 0.5f);
        TEST_ASSERT(center->layout.frame.x >= 150);

        auto* strip = gran::ui::dockStrip(*host, gran::ui::DockSide::LEFT);
        TEST_ASSERT(strip->children[0]->visible);
        TEST_ASSERT(strip->children[0]->content.checked);
        TEST_ASSERT(strip->children[1]->visible);
        TEST_ASSERT(!strip->children[2]->visible);
    }

    void testChooseAndMove()
    {
        auto doc = support::layoutMarkup(MARKUP, 800, 600);
        auto* host = gran::ui::findNode(*doc.root, "host");
        auto* left1 = gran::ui::findNode(*doc.root, "left1");
        auto* left2 = gran::ui::findNode(*doc.root, "left2");
        auto* bottom = gran::ui::findNode(*doc.root, "bottom");

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        auto* title2 = gran::ui::dockStrip(*host, gran::ui::DockSide::LEFT)->children[1].get();
        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {title2->layout.frame.x + 8, title2->layout.frame.y + title2->layout.frame.height / 2};
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_UP;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {title2->layout.frame.x + 8, title2->layout.frame.y + title2->layout.frame.height / 2};
        router.handle(event);
        support::FixedMeasurer relayout;
        gran::layout::layoutTree(*doc.root, gran::Size{800, 600}, relayout);

        TEST_ASSERT(left2->visible);
        TEST_ASSERT(!left1->visible);

        auto* titleB = gran::ui::dockStrip(*host, gran::ui::DockSide::BOTTOM)->children[2].get();
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {titleB->layout.frame.x + 8, titleB->layout.frame.y + titleB->layout.frame.height / 2};
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {780, 300};
        router.handle(event);

        TEST_ASSERT(host->content.dock.dropSide == 1);

        event.kind = gran::platform::EventKind::MOUSE_UP;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {780, 300};
        router.handle(event);

        TEST_ASSERT(bottom->content.dock.side == gran::ui::DockSide::RIGHT);
        TEST_ASSERT(sink.lastAction == "changed");
        TEST_ASSERT(host->content.dock.dropSide == -1);

        support::FixedMeasurer m;
        gran::layout::layoutTree(*doc.root, gran::Size{800, 600}, m);

        TEST_ASSERT(bottom->visible);
        TEST_ASSERT_NEAR(bottom->layout.frame.x + bottom->layout.frame.width, 800.0f, 0.5f);
    }

    void testHideShow()
    {
        auto doc = support::layoutMarkup(MARKUP, 800, 600);
        auto* host = gran::ui::findNode(*doc.root, "host");
        auto* left1 = gran::ui::findNode(*doc.root, "left1");
        auto* left2 = gran::ui::findNode(*doc.root, "left2");

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        auto* title1 = gran::ui::dockStrip(*host, gran::ui::DockSide::LEFT)->children[0].get();
        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {title1->layout.frame.x + title1->layout.frame.width - 8, title1->layout.frame.y + title1->layout.frame.height / 2};
        router.handle(event);

        TEST_ASSERT(left1->content.dock.hidden == true);

        support::FixedMeasurer m;
        gran::layout::layoutTree(*doc.root, gran::Size{800, 600}, m);

        TEST_ASSERT(left2->visible);

        left1->content.dock.hidden = false;
        gran::ui::chooseDockPanel(*left1);

        support::FixedMeasurer m2;
        gran::layout::layoutTree(*doc.root, gran::Size{800, 600}, m2);

        TEST_ASSERT(left1->visible);
        TEST_ASSERT(!left2->visible);
    }

    void testHandle()
    {
        auto doc = support::layoutMarkup(MARKUP, 800, 600);
        auto* host = gran::ui::findNode(*doc.root, "host");
        auto* left1 = gran::ui::findNode(*doc.root, "left1");

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        auto* h = gran::ui::dockHandle(*host, gran::ui::DockSide::LEFT);
        auto c = gran::Vec2{h->layout.frame.x + h->layout.frame.width / 2, h->layout.frame.y + h->layout.frame.height / 2};

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = c;
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {c.x + 50, c.y};
        router.handle(event);

        support::FixedMeasurer m;
        gran::layout::layoutTree(*doc.root, gran::Size{800, 600}, m);

        TEST_ASSERT_NEAR(left1->layout.frame.width, 202.0f, 4.0f);

        event.kind = gran::platform::EventKind::MOUSE_UP;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {c.x + 50, c.y};
        router.handle(event);

        TEST_ASSERT(sink.lastAction == "changed");
    }
}

namespace suites
{
    void registerDockTests(test::TestRunner& runner)
    {
        runner.add("dock: parts", testParts);
        runner.add("dock: layout", testLayout);
        runner.add("dock: choose and move", testChooseAndMove);
        runner.add("dock: hide show", testHideShow);
        runner.add("dock: handle", testHandle);
    }
}
