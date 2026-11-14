#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include "support/layout_helpers.hpp"
#include <vector>
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

        void event(const std::string& name, gran::ui::Node& node, gran::ui::UiEvent& event) override
        {
            log.emplace_back(name + ":" + node.id + ":" + std::to_string(static_cast<int>(event.phase)));
            if (name == "stop")
            {
                event.stopPropagation();
            }
            else if (name == "prevent")
            {
                event.preventDefault();
            }
        }

        std::string lastAction;
        int actionCount = 0;
        std::vector<std::string> log;
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

    void testEventRouting()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack id=\"outer\" onPointerDownCapture=\"cap\" onPointerDown=\"bubble\"><Box id=\"inner\" width=\"100\" height=\"40\" onPointerDown=\"target\"/></Stack></Window>",
            400, 300);

        auto* inner = gran::ui::findNode(*doc.root, "inner");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.position = {inner->layout.frame.x + inner->layout.frame.width / 2, inner->layout.frame.y + inner->layout.frame.height / 2};
        router.handle(event);

        TEST_ASSERT((sink.log == std::vector<std::string>{"cap:outer:1", "target:inner:2", "bubble:outer:3"}));
    }

    void testEventStopPropagation()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack id=\"outer\" onPointerDownCapture=\"cap\" onPointerDown=\"bubble\"><Box id=\"inner\" width=\"100\" height=\"40\" onPointerDown=\"stop\"/></Stack></Window>",
            400, 300);

        auto* inner = gran::ui::findNode(*doc.root, "inner");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.position = {inner->layout.frame.x + inner->layout.frame.width / 2, inner->layout.frame.y + inner->layout.frame.height / 2};
        router.handle(event);

        TEST_ASSERT((sink.log == std::vector<std::string>{"cap:outer:1", "stop:inner:2"}));
    }

    // Нажатие кнопки: отменённый pointerDown фокус не ставит / A button press: a cancelled pointerDown does not set the focus
    bool pressGivesFocus(const char* markup)
    {
        auto doc = support::layoutMarkup(markup, 400, 300);
        auto* b = gran::ui::findNode(*doc.root, "b");
        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;
        gran::input::InputRouter router(*doc.root, sink, &window, locator);
        gran::platform::Event event;

        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.position = {b->layout.frame.x + b->layout.frame.width / 2, b->layout.frame.y + b->layout.frame.height / 2};
        router.handle(event);
        return router.focused() == b;
    }

    void testEventPreventDefault()
    {
        TEST_ASSERT(!pressGivesFocus(
            "<Window><Button id=\"b\" text=\"B\" width=\"80\" height=\"30\" onPointerDown=\"prevent\"/></Window>"));
        TEST_ASSERT(pressGivesFocus("<Window><Button id=\"b\" text=\"B\" width=\"80\" height=\"30\"/></Window>"));
    }

    void testEventPreventDefaultInput()
    {
        auto doc = support::layoutMarkup(
            "<Window><Input id=\"field\" width=\"100\" height=\"30\" onKeyDown=\"prevent\" onKeyUp=\"up\"/></Window>",
            400, 300);

        auto* field = gran::ui::findNode(*doc.root, "field");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.focus(field);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::A;
        router.handle(event);

        TEST_ASSERT(field->content.text.empty());
        TEST_ASSERT((sink.log == std::vector<std::string>{"prevent:field:2"}));

        event.kind = gran::platform::EventKind::CHARACTER;
        event.character = U'a';
        router.handle(event);

        TEST_ASSERT(field->content.text.empty());

        event.kind = gran::platform::EventKind::KEY_UP;
        event.key = gran::platform::Key::A;
        router.handle(event);

        TEST_ASSERT((sink.log == std::vector<std::string>{"prevent:field:2", "up:field:2"}));
    }

    void testEventFocusBlur()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b1\" text=\"B1\" width=\"80\" height=\"30\" onFocus=\"f\" onBlur=\"bl\"/><Button id=\"b2\" text=\"B2\" width=\"80\" height=\"30\" onFocus=\"f\" onBlur=\"bl\"/></Stack></Window>",
            400, 300);

        auto* b1 = gran::ui::findNode(*doc.root, "b1");
        auto* b2 = gran::ui::findNode(*doc.root, "b2");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.focus(b1);

        TEST_ASSERT((sink.log == std::vector<std::string>{"f:b1:2"}));

        sink.log.clear();

        router.focus(b2);

        TEST_ASSERT((sink.log == std::vector<std::string>{"bl:b1:2", "f:b2:2"}));
    }

    void testEventPointerEnterLeave()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack id=\"outer\" onPointerEnter=\"en\" onPointerLeave=\"lv\"><Box id=\"a\" width=\"100\" height=\"40\" onPointerEnter=\"en\" onPointerLeave=\"lv\"/><Box id=\"c\" width=\"100\" height=\"40\" onPointerEnter=\"en\" onPointerLeave=\"lv\"/></Stack></Window>",
            400, 300);

        auto* a = gran::ui::findNode(*doc.root, "a");
        auto* c = gran::ui::findNode(*doc.root, "c");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        event.position = {a->layout.frame.x + a->layout.frame.width / 2, a->layout.frame.y + a->layout.frame.height / 2};
        router.handle(event);

        TEST_ASSERT((sink.log == std::vector<std::string>{"en:outer:2", "en:a:2"}));

        sink.log.clear();

        event.position = {c->layout.frame.x + c->layout.frame.width / 2, c->layout.frame.y + c->layout.frame.height / 2};
        router.handle(event);

        TEST_ASSERT((sink.log == std::vector<std::string>{"lv:a:2", "en:c:2"}));

        sink.log.clear();

        event.kind = gran::platform::EventKind::MOUSE_LEAVE;
        router.handle(event);

        TEST_ASSERT((sink.log == std::vector<std::string>{"lv:c:2", "lv:outer:2"}));
    }

    void testCustomEvent()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack id=\"outer\" on=\"ping:got\"><Box id=\"inner\" width=\"100\" height=\"40\"/></Stack></Window>",
            400, 300);

        auto* outer = gran::ui::findNode(*doc.root, "outer");
        auto* inner = gran::ui::findNode(*doc.root, "inner");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::ui::EventListener l;
        l.type = gran::ui::EventType::CUSTOM;
        l.name = "ping";
        l.id = 1;
        int count = 0;
        l.handler = [&count](gran::ui::Node&, gran::ui::UiEvent& event) {
            TEST_ASSERT(event.detail == "hello");
            count++;
        };
        outer->listeners.push_back(l);

        auto e = gran::ui::makeEvent(gran::ui::EventType::CUSTOM, *inner, "ping");
        e.detail = "hello";

        TEST_ASSERT(router.dispatch(e));
        TEST_ASSERT((sink.log == std::vector<std::string>{"got:outer:3"}));
        TEST_ASSERT(count == 1);
    }

    void testEventAttributes()
    {
        auto doc = support::layoutMarkup(
            "<Window><Box id=\"x\" onKeyDownCapture=\"k\" on=\"a:x b:gran:close\"/></Window>",
            400, 300);

        auto* x = gran::ui::findNode(*doc.root, "x");

        TEST_ASSERT(x->content.events.size() == 3);

        TEST_ASSERT(x->content.events[0].type == gran::ui::EventType::KEY_DOWN);
        TEST_ASSERT(x->content.events[0].capture == true);
        TEST_ASSERT(x->content.events[0].action == "k");

        TEST_ASSERT(x->content.events[1].type == gran::ui::EventType::CUSTOM);
        TEST_ASSERT(x->content.events[1].name == "a");
        TEST_ASSERT(x->content.events[1].action == "x");

        TEST_ASSERT(x->content.events[2].type == gran::ui::EventType::CUSTOM);
        TEST_ASSERT(x->content.events[2].name == "b");
        TEST_ASSERT(x->content.events[2].action == "gran:close");
    }
}

namespace suites
{
    void registerEventRoutingTests(test::TestRunner& runner)
    {
        runner.add("events: routing", testEventRouting);
        runner.add("events: stop propagation", testEventStopPropagation);
        runner.add("events: prevent default", testEventPreventDefault);
        runner.add("events: prevent default input", testEventPreventDefaultInput);
        runner.add("events: focus blur", testEventFocusBlur);
        runner.add("events: pointer enter leave", testEventPointerEnterLeave);
        runner.add("events: custom event", testCustomEvent);
        runner.add("events: attributes", testEventAttributes);
    }
}
