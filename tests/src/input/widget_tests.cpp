#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include "support/layout_helpers.hpp"
#include <cmath>
#include <functional>
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

    void testRadioButtonGroup()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><RadioButton id=\"r1\" text=\"One\" checked=\"true\" onChange=\"pick\" width=\"80\" height=\"24\"/><RadioButton id=\"r2\" text=\"Two\" onChange=\"pick\" width=\"80\" height=\"24\"/><RadioButton id=\"r3\" text=\"Three\" onChange=\"pick\" width=\"80\" height=\"24\"/></Stack></Window>",
            400, 300);

        auto* r1 = gran::ui::findNode(*doc.root, "r1");
        auto* r2 = gran::ui::findNode(*doc.root, "r2");
        auto* r3 = gran::ui::findNode(*doc.root, "r3");

        TEST_ASSERT(r1->content.checked == true);
        TEST_ASSERT(r2->content.checked == false);
        TEST_ASSERT(r3->content.checked == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {r2->layout.frame.x + r2->layout.frame.width / 2, r2->layout.frame.y + r2->layout.frame.height / 2};
        router.handle(event);
        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);

        TEST_ASSERT(r1->content.checked == false);
        TEST_ASSERT(r2->content.checked == true);
        TEST_ASSERT(r3->content.checked == false);
        TEST_ASSERT(sink.lastAction == "pick");
        TEST_ASSERT(sink.actionCount == 1);

        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {r2->layout.frame.x + r2->layout.frame.width / 2, r2->layout.frame.y + r2->layout.frame.height / 2};
        router.handle(event);
        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);

        TEST_ASSERT(sink.actionCount == 1);
    }

    void testRadioButtonNavigation()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><RadioButton id=\"r1\" text=\"One\" checked=\"true\" onChange=\"pick\" width=\"80\" height=\"24\"/><RadioButton id=\"r2\" text=\"Two\" onChange=\"pick\" width=\"80\" height=\"24\"/><RadioButton id=\"r3\" text=\"Three\" onChange=\"pick\" width=\"80\" height=\"24\"/></Stack></Window>",
            400, 300);

        auto* r1 = gran::ui::findNode(*doc.root, "r1");
        auto* r2 = gran::ui::findNode(*doc.root, "r2");
        auto* r3 = gran::ui::findNode(*doc.root, "r3");

        TEST_ASSERT(r1->content.checked == true);
        TEST_ASSERT(r2->content.checked == false);
        TEST_ASSERT(r3->content.checked == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.focus(r1);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::DOWN;
        router.handle(event);

        TEST_ASSERT(router.focused() == r2);
        TEST_ASSERT(r2->content.checked == true);
        TEST_ASSERT(r1->content.checked == false);
        TEST_ASSERT(r3->content.checked == false);
        TEST_ASSERT(sink.lastAction == "pick");
        TEST_ASSERT(sink.actionCount == 1);

        event.key = gran::platform::Key::DOWN;
        router.handle(event);

        TEST_ASSERT(router.focused() == r3);
        TEST_ASSERT(r3->content.checked == true);
        TEST_ASSERT(r1->content.checked == false);
        TEST_ASSERT(r2->content.checked == false);
        TEST_ASSERT(sink.lastAction == "pick");
        TEST_ASSERT(sink.actionCount == 2);

        event.key = gran::platform::Key::DOWN;
        router.handle(event);

        TEST_ASSERT(router.focused() == r1);
        TEST_ASSERT(r1->content.checked == true);
        TEST_ASSERT(r2->content.checked == false);
        TEST_ASSERT(r3->content.checked == false);
        TEST_ASSERT(sink.lastAction == "pick");
        TEST_ASSERT(sink.actionCount == 3);
    }

    void testRadioButtonGroupByName()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Stack><RadioButton id=\"a\" text=\"A\" group=\"g\" checked=\"true\" onChange=\"pick\" width=\"80\" height=\"24\"/><RadioButton id=\"b\" text=\"B\" group=\"g\" onChange=\"pick\" width=\"80\" height=\"24\"/></Stack><Stack><RadioButton id=\"c\" text=\"C\" group=\"g\" onChange=\"pick\" width=\"80\" height=\"24\"/></Stack></Stack></Window>",
            400, 300);

        auto* a = gran::ui::findNode(*doc.root, "a");
        auto* b = gran::ui::findNode(*doc.root, "b");
        auto* c = gran::ui::findNode(*doc.root, "c");

        TEST_ASSERT(a->content.checked == true);
        TEST_ASSERT(b->content.checked == false);
        TEST_ASSERT(c->content.checked == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {c->layout.frame.x + c->layout.frame.width / 2, c->layout.frame.y + c->layout.frame.height / 2};
        router.handle(event);
        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);

        TEST_ASSERT(a->content.checked == false);
        TEST_ASSERT(b->content.checked == false);
        TEST_ASSERT(c->content.checked == true);
        TEST_ASSERT(sink.lastAction == "pick");
        TEST_ASSERT(sink.actionCount == 1);
    }

    void testRadioButtonTabOrder()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><RadioButton id=\"r1\" text=\"One\" width=\"80\" height=\"24\"/><RadioButton id=\"r2\" text=\"Two\" checked=\"true\" width=\"80\" height=\"24\"/><RadioButton id=\"r3\" text=\"Three\" width=\"80\" height=\"24\"/><Button id=\"btn\" text=\"Button\" width=\"80\" height=\"24\"/></Stack></Window>",
            400, 300);

        auto* r2 = gran::ui::findNode(*doc.root, "r2");
        auto* btn = gran::ui::findNode(*doc.root, "btn");

        TEST_ASSERT(r2->content.checked == true);

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::TAB;
        router.handle(event);

        TEST_ASSERT(router.focused() == r2);

        event.key = gran::platform::Key::TAB;
        router.handle(event);

        TEST_ASSERT(router.focused() == btn);
    }

    void testSwitchBehavior()
    {
        auto doc = support::layoutMarkup(
            "<Window><Switch id=\"sw\" text=\"x\" onChange=\"sw\" width=\"80\" height=\"24\"/></Window>",
            400, 300);

        auto* sw = gran::ui::findNode(*doc.root, "sw");

        TEST_ASSERT(sw->content.checked == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.focus(sw);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::SPACE;
        router.handle(event);

        TEST_ASSERT(sw->content.checked == true);
        TEST_ASSERT(sink.lastAction == "sw");
        TEST_ASSERT(sink.actionCount == 1);

        event.key = gran::platform::Key::SPACE;
        router.handle(event);

        TEST_ASSERT(sw->content.checked == false);
        TEST_ASSERT(sink.lastAction == "sw");
        TEST_ASSERT(sink.actionCount == 2);
    }

    void testTabsBehavior()
    {
        auto doc = support::layoutMarkup(
            "<Window><Tabs id=\"t\" value=\"b\" onChange=\"tab\"><Tab id=\"pa\" text=\"A\"><Button text=\"1\"/></Tab><Tab id=\"pb\" text=\"B\" value=\"b\"><Button text=\"2\"/></Tab></Tabs></Window>",
            400, 300);

        auto* t = gran::ui::findNode(*doc.root, "t");
        auto* pa = gran::ui::findNode(*doc.root, "pa");
        auto* pb = gran::ui::findNode(*doc.root, "pb");

        TEST_ASSERT(pb->visible == true);
        TEST_ASSERT(pa->visible == false);
        TEST_ASSERT(t->children[0]->kind == gran::ui::ElementKind::TAB_BAR);
        TEST_ASSERT(t->children[0]->children[1]->content.checked == true);

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.focus(t->children[0]->children[1].get());

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::LEFT;
        router.handle(event);

        TEST_ASSERT(pa->visible == true);
        TEST_ASSERT(pb->visible == false);
        TEST_ASSERT(t->content.option == "A");
        TEST_ASSERT(sink.lastAction == "tab");
        TEST_ASSERT(router.focused() == t->children[0]->children[0].get());
    }

    void testTabsNoValue()
    {
        auto doc = support::layoutMarkup(
            "<Window><Tabs id=\"t\"><Tab id=\"pa\" text=\"A\"><Button text=\"1\"/></Tab><Tab id=\"pb\" text=\"B\"><Button text=\"2\"/></Tab></Tabs></Window>",
            400, 300);

        auto* t = gran::ui::findNode(*doc.root, "t");
        auto* pa = gran::ui::findNode(*doc.root, "pa");
        auto* pb = gran::ui::findNode(*doc.root, "pb");

        TEST_ASSERT(pa->visible == true);
        TEST_ASSERT(pb->visible == false);

        TEST_ASSERT(gran::ui::selectTab(*t, "нет такой") == false);
        TEST_ASSERT(pa->visible == true);
        TEST_ASSERT(pb->visible == false);
    }
}

namespace suites
{
    void registerWidgetTests(test::TestRunner& runner)
    {
        runner.add("widgets: radio button group", testRadioButtonGroup);
        runner.add("widgets: radio button navigation", testRadioButtonNavigation);
        runner.add("widgets: radio button group by name", testRadioButtonGroupByName);
        runner.add("widgets: radio button tab order", testRadioButtonTabOrder);
        runner.add("widgets: switch behavior", testSwitchBehavior);
        runner.add("widgets: tabs behavior", testTabsBehavior);
        runner.add("widgets: tabs no value", testTabsNoValue);
    }
}
