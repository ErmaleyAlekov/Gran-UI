#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/animation/animator.hpp"
#include "gran/markup/theme.hpp"
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
            else if (name == "animatedUp")
            {
                TEST_ASSERT_NEAR(event.local.x, 22.0f, 0.001f);
                TEST_ASSERT_NEAR(event.local.y, 22.0f, 0.001f);
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

    int clickDuringTransform(const char* minimum, float releaseX, bool animateParent)
    {
        using namespace gran;
        const std::string transform = R"xml( transform="scale(1,1)" transition="transform 1s linear">)xml";
        const std::string pressed = R"xml(<Pressed transform="scale(-1,1)"/>)xml";
        const auto button = R"(<Button id="target" width="44" height="44" text="Click" onClick="act" onPointerUp="animatedUp")";
        const auto body = animateParent
            ? "<Stack id=\"parent\" width=\"44\" height=\"44\"" + transform + pressed + button + R"xml(><Pressed transform="none"/></Button></Stack>)xml"
            : button + transform + pressed + "</Button>";
        auto doc = markup::loadMarkupString(std::string("<Window") + minimum + ">" + body + "</Window>", {});
        auto& target = *ui::findNode(*doc.root, "target");
        auto& animated = animateParent ? *doc.root : target;
        target.layout.frame = animated.layout.frame = {0, 0, 44, 44};
        const auto& theme = markup::defaultTheme();
        const auto windowStyle = markup::windowStyle(theme, doc.window);
        animation::Animator animator;
        ActionSinkStub sink;
        TextLocatorStub locator;
        platform::HeadlessWindow window;
        input::InputRouter router(*doc.root, sink, &window, locator);

        animator.setTime(0);
        ui::resolveStyles(*doc.root, windowStyle, theme.kinds, &animator);
        platform::Event event;
        event.kind = platform::EventKind::MOUSE_DOWN;
        event.button = platform::MouseButton::LEFT;
        event.position = {22, 22};
        router.handle(event);
        if (animateParent)
            animated.stateMask = style::stateBit(style::State::PRESSED);
        ui::markDirty(animated, ui::DIRTY_STYLE);
        ui::resolveStyles(*doc.root, windowStyle, theme.kinds, &animator);
        animator.setTime(0.5);
        animator.tick();
        TEST_ASSERT(ui::nodeTransform(animated).a == 0);
        event.kind = platform::EventKind::MOUSE_UP;
        event.position.x = releaseX;
        router.handle(event);
        return sink.actionCount;
    }

    void testAnimatedTargetClick()
    {
        const auto minimum = R"( minTargetSize="44")";
        TEST_ASSERT(clickDuringTransform(minimum, 22, false) == 1);
        TEST_ASSERT(clickDuringTransform(minimum, 22, true) == 1);
        TEST_ASSERT(clickDuringTransform(minimum, 45, false) == 0);
        TEST_ASSERT(clickDuringTransform("", 22, false) == 0);
    }

    void testExpandedTargetClipsAndStacks()
    {
        using namespace gran;
        auto doc = markup::loadMarkupString(R"(<Window minTargetSize="44"><Stack>
            <Stack id="clip" overflow="hidden"><Button id="target" text="Target"/></Stack>
            <Box id="cover"/>
            </Stack></Window>)", {});
        auto& clip = *ui::findNode(*doc.root, "clip");
        auto& target = *ui::findNode(*doc.root, "target");
        auto& cover = *ui::findNode(*doc.root, "cover");
        clip.layout.frame = {0, 0, 34, 44};
        target.layout.frame = {0, 0, 44, 44};
        cover.layout.frame = {20, 0, 14, 44};
        const auto& theme = markup::defaultTheme();
        ui::resolveStyles(*doc.root, markup::windowStyle(theme, doc.window), theme.kinds, nullptr);
        target.computed.transform = {{style::TransformKind::SCALE, 0, 1}};
        TEST_ASSERT(input::hitTest(*doc.root, {12, 22}) == &target);
        TEST_ASSERT(input::hitTest(*doc.root, {25, 22}) == &cover);
        TEST_ASSERT(input::hitTest(*doc.root, {40, 22}) != &target);
        target.computed.transform = {{style::TransformKind::SCALE, 0.5f, 1}};
        TEST_ASSERT(input::hitTest(*doc.root, {10, 22}) == &target);
        TEST_ASSERT(input::hitTest(*doc.root, {11, 22}) == &target);
        const auto expanded = ui::pointerToNode(target, {10, 22});
        const auto edge = ui::pointerToNode(target, {11, 22});
        TEST_ASSERT_NEAR(expanded.x, edge.x, 0.001f);
        TEST_ASSERT_NEAR(expanded.x, target.layout.frame.x, 0.001f);
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
        runner.add("events: minimum target click during a singular transform", testAnimatedTargetClick);
        runner.add("events: expanded hit target preserves clip and stacking", testExpandedTargetClipsAndStacks);
        runner.add("events: wheel", testWheel);
        runner.add("events: pointer capture", testPointerCapture);
        runner.add("events: drag and drop", testDragAndDrop);
        runner.add("events: drag cancel", testDragCancel);
        runner.add("events: key name", testKeyName);
        runner.add("events: C API", testCapi);
    }
}
