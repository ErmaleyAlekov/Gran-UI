#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include "support/layout_helpers.hpp"
#include "gran/capi/gran.h"
#include "gran/style/style_sheet.hpp"
#include "gran/platform/headless/headless_window.hpp"
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
            if (name == "dbl")
            {
                TEST_ASSERT(event.clicks == 2);
                doubleClickHandled = true;
            }
            else if (name == "prevent")
            {
                event.preventDefault();
            }
            else if (name == "mv")
            {
                TEST_ASSERT(event.current->id == "pad");
            }
            else if (name == "start")
            {
                TEST_ASSERT(event.current->id == "card");
                dragData = event.drag.data;
            }
            else if (name == "enter")
            {
                TEST_ASSERT(event.current->id == "zone");
                TEST_ASSERT(event.related->id == "card");
            }
            else if (name == "over")
            {
                TEST_ASSERT(event.current->id == "zone");
                TEST_ASSERT(event.related->id == "card");
            }
            else if (name == "drop")
            {
                TEST_ASSERT(event.current->id == "zone");
                TEST_ASSERT(event.related->id == "card");
                dragData = event.drag.data;
            }
            else if (name == "end")
            {
                // После отмены зоны нет: related — зона только при сбросе / After a cancel there is no zone: related is the zone only on a drop
                TEST_ASSERT(event.current->id == "card");
                TEST_ASSERT(event.related == nullptr || event.related->id == "zone");
                dropped = event.dropped;
                endHandled = true;
            }
        }

        std::string lastAction;
        int actionCount = 0;
        std::vector<std::string> log;
        bool doubleClickHandled = false;
        std::string dragData;
        bool dropped = false;
        bool endHandled = false;
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

    void testDoubleClick()
    {
        auto doc = support::layoutMarkup(
            "<Window><Box id=\"b\" width=\"100\" height=\"40\" onDoubleClick=\"dbl\"/></Window>",
            400, 300);

        auto* b = gran::ui::findNode(*doc.root, "b");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.position = {b->layout.frame.x + b->layout.frame.width / 2, b->layout.frame.y + b->layout.frame.height / 2};
        event.clicks = 1;
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.clicks = 2;
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);

        TEST_ASSERT((sink.log == std::vector<std::string>{"dbl:b:2"}));
        TEST_ASSERT(sink.doubleClickHandled);
    }

    // Сдвиг прокрутки после одного щелчка колеса / The scroll shift after one wheel notch
    float wheelShift(const char* markup)
    {
        auto doc = support::layoutMarkup(markup, 400, 300);
        auto* s = gran::ui::findNode(*doc.root, "s");
        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;
        gran::input::InputRouter router(*doc.root, sink, &window, locator);
        gran::platform::Event event;

        event.kind = gran::platform::EventKind::MOUSE_WHEEL;
        event.position = {s->layout.frame.x + s->layout.frame.width / 2, s->layout.frame.y + s->layout.frame.height / 2};
        event.wheel.y = -1;
        router.handle(event);
        return s->layout.scrollOffset.y;
    }

    void testWheel()
    {
        TEST_ASSERT(wheelShift("<Window><Scroll id=\"s\" height=\"60\" onWheel=\"prevent\"><Box height=\"300\"/></Scroll></Window>") == 0);
        TEST_ASSERT(wheelShift("<Window><Scroll id=\"s\" height=\"60\"><Box height=\"300\"/></Scroll></Window>") != 0);
    }

    void testPointerCapture()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Box id=\"pad\" width=\"100\" height=\"40\" onPointerMove=\"mv\"/><Box id=\"other\" width=\"100\" height=\"40\"/></Stack></Window>",
            400, 300);

        auto* pad = gran::ui::findNode(*doc.root, "pad");
        auto* other = gran::ui::findNode(*doc.root, "other");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.capturePointer(pad);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        event.position = {other->layout.frame.x + other->layout.frame.width / 2, other->layout.frame.y + other->layout.frame.height / 2};
        router.handle(event);

        TEST_ASSERT((sink.log == std::vector<std::string>{"mv:pad:2"}));
        TEST_ASSERT(router.pointerCapture() == pad);

        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);

        TEST_ASSERT(router.pointerCapture() == nullptr);
    }

    void testDragAndDrop()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Box id=\"card\" width=\"80\" height=\"30\" draggable=\"true\" dragData=\"c1\" onDragStart=\"start\" onDragEnd=\"end\"/><Box id=\"zone\" width=\"150\" height=\"80\" acceptDrop=\"node\" onDragEnter=\"enter\" onDragOver=\"over\" onDrop=\"drop\"/></Stack></Window>",
            400, 300);

        auto* card = gran::ui::findNode(*doc.root, "card");
        auto* zone = gran::ui::findNode(*doc.root, "zone");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.position = {card->layout.frame.x + card->layout.frame.width / 2, card->layout.frame.y + card->layout.frame.height / 2};
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        event.position = {zone->layout.frame.x + zone->layout.frame.width / 2, zone->layout.frame.y + zone->layout.frame.height / 2};
        router.handle(event);

        TEST_ASSERT(router.draggingNode());
        TEST_ASSERT((sink.log == std::vector<std::string>{"start:card:2", "enter:zone:2", "over:zone:2"}));
        TEST_ASSERT(zone->stateMask & gran::style::stateBit(gran::style::State::DROP_TARGET));

        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);

        TEST_ASSERT((sink.log == std::vector<std::string>{"start:card:2", "enter:zone:2", "over:zone:2", "drop:zone:2", "end:card:2"}));
        TEST_ASSERT(sink.dragData == "c1");
        TEST_ASSERT(sink.dropped);
        TEST_ASSERT(!router.draggingNode());
    }

    void testDragCancel()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Box id=\"card\" width=\"80\" height=\"30\" draggable=\"true\" dragData=\"c1\" onDragStart=\"start\" onDragEnd=\"end\"/><Box id=\"zone\" width=\"150\" height=\"80\" acceptDrop=\"node\" onDragEnter=\"enter\" onDragOver=\"over\" onDrop=\"drop\"/></Stack></Window>",
            400, 300);

        auto* card = gran::ui::findNode(*doc.root, "card");
        auto* zone = gran::ui::findNode(*doc.root, "zone");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.position = {card->layout.frame.x + card->layout.frame.width / 2, card->layout.frame.y + card->layout.frame.height / 2};
        router.handle(event);

        event.kind = gran::platform::EventKind::MOUSE_MOVE;
        event.position = {zone->layout.frame.x + zone->layout.frame.width / 2, zone->layout.frame.y + zone->layout.frame.height / 2};
        router.handle(event);

        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ESCAPE;
        router.handle(event);

        TEST_ASSERT(!router.draggingNode());
        TEST_ASSERT((sink.log == std::vector<std::string>{"start:card:2", "enter:zone:2", "over:zone:2", "end:card:2"}));
        TEST_ASSERT(sink.endHandled);
        TEST_ASSERT(!sink.dropped);
    }

    void testKeyName()
    {
        TEST_ASSERT(std::string(gran::platform::keyName(gran::platform::Key::F5)) == "F5");
        TEST_ASSERT(std::string(gran::platform::keyName(gran::platform::Key::LEFT)) == "ArrowLeft");
        TEST_ASSERT(std::string(gran::platform::keyName(gran::platform::Key::A)) == "A");
        TEST_ASSERT(std::string(gran::platform::keyName(gran::platform::Key::DIGIT_7)) == "7");
        TEST_ASSERT(std::string(gran::platform::keyName(gran::platform::Key::UNKNOWN)) == "Unidentified");
    }

    // Копии полей снимка: его строки живут только внутри callback / Copies of the snapshot fields: its strings live only inside the callback
    struct Seen
    {
        std::string type;
        std::string target;
        std::string current;
        int32_t phase = 0;
        std::string detail;
    };

    int32_t recordEvent(void* user, const gran_input_event* event)
    {
        auto* seen = static_cast<Seen*>(user);
        seen->type = event->type;
        seen->target = event->target;
        seen->current = event->current;
        seen->phase = event->phase;
        seen->detail = event->detail;
        return GRAN_EVENT_PREVENT_DEFAULT;
    }

    void testCapi()
    {
        gran_app* app = nullptr;
        TEST_ASSERT(gran_app_create(nullptr, &app) == GRAN_OK);
        TEST_ASSERT(gran_app_load(app, "<Window><Stack id=\"root\"><Button id=\"go\" text=\"Go\"/></Stack></Window>", 0) == GRAN_OK);

        uint64_t id = 0;
        Seen seen;
        TEST_ASSERT(gran_app_listen(app, "root", "ping", 0, recordEvent, &seen, &id) == GRAN_OK);
        TEST_ASSERT(id != 0);

        int32_t allowed = 1;
        TEST_ASSERT(gran_app_dispatch(app, "go", "ping", "data", &allowed) == GRAN_OK);
        TEST_ASSERT(allowed == 0);
        TEST_ASSERT(seen.type == "ping");
        TEST_ASSERT(seen.target == "go");
        TEST_ASSERT(seen.current == "root");
        TEST_ASSERT(seen.phase == 3);
        TEST_ASSERT(seen.detail == "data");

        TEST_ASSERT(gran_app_unlisten(app, "root", id) == GRAN_OK);
        TEST_ASSERT(gran_app_dispatch(app, "go", "ping", "data", &allowed) == GRAN_OK);
        TEST_ASSERT(allowed == 1);

        TEST_ASSERT(gran_app_destroy(app) == GRAN_OK);
    }
}

namespace suites
{
    void registerEventInputTests(test::TestRunner& runner)
    {
        runner.add("events: double click", testDoubleClick);
        runner.add("events: wheel", testWheel);
        runner.add("events: pointer capture", testPointerCapture);
        runner.add("events: drag and drop", testDragAndDrop);
        runner.add("events: drag cancel", testDragCancel);
        runner.add("events: key name", testKeyName);
        runner.add("events: C API", testCapi);
    }
}
