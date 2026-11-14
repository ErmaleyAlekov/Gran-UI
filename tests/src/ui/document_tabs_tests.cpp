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
                <Tabs id="docs" closable="true" reorderable="true" onClose="closed" onReorder="moved" onChange="changed">
                    <Tab id="a" text="Один"/>
                    <Tab id="b" text="Два"/>
                    <Tab id="c" text="Три"/>
                </Tabs>
            </Stack>
        </Window>
    )";

    void testHeaders()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* docs = gran::ui::findNode(*doc.root, "docs");
        auto* bar = docs->children[0].get();

        TEST_ASSERT(bar->children.size() == 3);
        TEST_ASSERT(gran::ui::headerOfPage(*gran::ui::findNode(*doc.root, "a")) == bar->children[0].get());
        TEST_ASSERT(gran::ui::pageOf(*bar->children[1]) == gran::ui::findNode(*doc.root, "b"));
        TEST_ASSERT(gran::ui::tabClosable(*bar->children[0]));
    }

    void testMoveTab()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* docs = gran::ui::findNode(*doc.root, "docs");
        auto* bar = docs->children[0].get();

        TEST_ASSERT(gran::ui::moveTab(*docs, 0, 2));
        TEST_ASSERT(bar->children[2]->content.text == u8"Один");
        TEST_ASSERT(docs->children[3].get() == gran::ui::findNode(*doc.root, "a"));
        TEST_ASSERT(gran::ui::pageOf(*bar->children[2]) == gran::ui::findNode(*doc.root, "a"));
        TEST_ASSERT(!gran::ui::moveTab(*docs, 1, 1));
    }

    void testCloseRequests()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* docs = gran::ui::findNode(*doc.root, "docs");
        auto* bar = docs->children[0].get();

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        auto* header1 = bar->children[0].get();
        auto* header2 = bar->children[1].get();

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::MIDDLE;
        event.position = {header1->layout.frame.x + header1->layout.frame.width / 2, header1->layout.frame.y + header1->layout.frame.height / 2};
        router.handle(event);
        TEST_ASSERT(sink.lastAction == "closed");
        sink.lastAction = "";

        router.focus(header1);
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::W;
        event.modifiers = gran::platform::MODIFIER_CONTROL;
        router.handle(event);
        TEST_ASSERT(sink.lastAction == "closed");
        sink.lastAction = "";

        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {header2->layout.frame.x + header2->layout.frame.width - 12, header2->layout.frame.y + header2->layout.frame.height / 2};
        router.handle(event);
        TEST_ASSERT(sink.lastAction == "closed");
    }

    void testCtrlTab()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* docs = gran::ui::findNode(*doc.root, "docs");
        auto* bar = docs->children[0].get();

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.focus(bar->children[0].get());

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::TAB;
        event.modifiers = gran::platform::MODIFIER_CONTROL;
        router.handle(event);
        TEST_ASSERT(docs->content.option == u8"Два" && sink.lastAction == "changed");
        sink.lastAction = "";

        event.modifiers = gran::platform::MODIFIER_CONTROL | gran::platform::MODIFIER_SHIFT;
        router.handle(event);
        TEST_ASSERT(docs->content.option == u8"Один");
        router.handle(event);
        TEST_ASSERT(docs->content.option == u8"Три");
    }

    void testReorderDrag()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* docs = gran::ui::findNode(*doc.root, "docs");
        auto* bar = docs->children[0].get();

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        auto* h0 = bar->children[0].get();
        auto* h1 = bar->children[1].get();

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {h0->layout.frame.x + 15, h0->layout.frame.y + h0->layout.frame.height / 2};
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {h1->layout.frame.x + h1->layout.frame.width * 0.75f, h1->layout.frame.y + h1->layout.frame.height / 2};
        router.handle(event);
        TEST_ASSERT(bar->children[1]->content.text == u8"Один");

        event.kind = gran::platform::EventKind::MOUSE_UP;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {h1->layout.frame.x + h1->layout.frame.width * 0.75f, h1->layout.frame.y + h1->layout.frame.height / 2};
        router.handle(event);
        TEST_ASSERT(sink.lastAction == "moved");
    }
}

namespace suites
{
    void registerDocumentTabsTests(test::TestRunner& runner)
    {
        runner.add("document tabs: headers", testHeaders);
        runner.add("document tabs: move tab", testMoveTab);
        runner.add("document tabs: close requests", testCloseRequests);
        runner.add("document tabs: ctrl tab", testCtrlTab);
        runner.add("document tabs: reorder drag", testReorderDrag);
    }
}
