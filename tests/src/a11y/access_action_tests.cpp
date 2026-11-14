#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/a11y/access_tree.hpp"
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

    void testAccessPatterns()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b\" text=\"Go\" onClick=\"go\" width=\"80\" height=\"24\"/><CheckBox id=\"c\" text=\"C\" onChange=\"chk\" width=\"80\" height=\"24\"/><Slider id=\"s\" min=\"0\" max=\"10\" step=\"2\" onChange=\"vol\" width=\"100\" height=\"20\"/><Input id=\"i\" maxLength=\"5\" onChange=\"txt\" width=\"100\" height=\"20\"/><Input id=\"ro\" value=\"keep\" readOnly=\"true\" width=\"100\" height=\"20\"/></Stack></Window>",
            400, 300);

        auto* b = gran::ui::findNode(*doc.root, "b");
        auto* c = gran::ui::findNode(*doc.root, "c");
        auto* s = gran::ui::findNode(*doc.root, "s");
        auto* i = gran::ui::findNode(*doc.root, "i");
        auto* ro = gran::ui::findNode(*doc.root, "ro");

        TEST_ASSERT(gran::a11y::patternsOf(*b) == gran::a11y::PATTERN_INVOKE);
        TEST_ASSERT(gran::a11y::patternsOf(*c) == gran::a11y::PATTERN_TOGGLE);
        TEST_ASSERT(gran::a11y::patternsOf(*s) == gran::a11y::PATTERN_RANGE);
        TEST_ASSERT(gran::a11y::patternsOf(*i) == gran::a11y::PATTERN_TEXT);
        TEST_ASSERT(gran::a11y::patternsOf(*ro) == gran::a11y::PATTERN_TEXT);

        gran::ui::Node node;
        node.kind = gran::ui::ElementKind::SWITCH;
        TEST_ASSERT(gran::a11y::patternsOf(node) == gran::a11y::PATTERN_TOGGLE);
        node.kind = gran::ui::ElementKind::RADIO_BUTTON;
        TEST_ASSERT(gran::a11y::patternsOf(node) == gran::a11y::PATTERN_SELECT);
        node.kind = gran::ui::ElementKind::PROGRESS_BAR;
        TEST_ASSERT(gran::a11y::patternsOf(node) == gran::a11y::PATTERN_RANGE);
        node.kind = gran::ui::ElementKind::STACK;
        TEST_ASSERT(gran::a11y::patternsOf(node) == 0);
        node.content.onClick = "go";
        TEST_ASSERT(gran::a11y::patternsOf(node) == gran::a11y::PATTERN_INVOKE);
        node.kind = gran::ui::ElementKind::INPUT;
        node.content.readOnly = true;
        TEST_ASSERT(gran::a11y::isReadOnly(node) == true);
    }

    void testAccessModal()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"under\" text=\"x\" onClick=\"under\" width=\"80\" height=\"30\"/><Dialog id=\"d\"><Button id=\"ok\" text=\"OK\" onClick=\"ok\"/></Dialog></Stack></Window>",
            400, 300);
        auto* under = gran::ui::findNode(*doc.root, "under");
        auto* d = gran::ui::findNode(*doc.root, "d");
        auto* ok = gran::ui::findNode(*doc.root, "ok");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;
        gran::input::InputRouter router(*doc.root, sink, &window, locator);
        router.openPopup(*d, gran::Rect{}, false);

        gran::a11y::AccessRequest request;
        request.kind = gran::a11y::Request::INVOKE;
        request.node = under;
        TEST_ASSERT(router.perform(request) == false);
        TEST_ASSERT(sink.actionCount == 0);
        request.node = ok;
        TEST_ASSERT(router.perform(request) == true);
        TEST_ASSERT(sink.lastAction == "ok");
    }

    void testAccessReadOnly()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Input id=\"i\" placeholder=\"Hint\" accessibleName=\"Login\" width=\"100\" height=\"20\"/><ComboBox id=\"cb\" width=\"100\" height=\"20\"/><ProgressBar id=\"pb\" width=\"100\" height=\"20\"/><Slider id=\"s\" min=\"0\" max=\"10\" step=\"2\" width=\"100\" height=\"20\"/></Stack></Window>",
            400, 300);

        auto* i = gran::ui::findNode(*doc.root, "i");
        auto* cb = gran::ui::findNode(*doc.root, "cb");
        auto* pb = gran::ui::findNode(*doc.root, "pb");
        auto* s = gran::ui::findNode(*doc.root, "s");

        TEST_ASSERT(gran::a11y::isReadOnly(*i) == false);
        TEST_ASSERT(gran::a11y::isReadOnly(*cb) == true);
        TEST_ASSERT(gran::a11y::isReadOnly(*pb) == true);
        TEST_ASSERT(gran::a11y::isReadOnly(*s) == false);
    }

    void testAccessNames()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Text id=\"lbl\">Volume</Text><Slider id=\"s\" labeledBy=\"lbl\" width=\"100\" height=\"20\"/><Input id=\"i\" placeholder=\"Hint\" accessibleName=\"Login\" width=\"100\" height=\"20\"/></Stack></Window>",
            400, 300);

        auto* s = gran::ui::findNode(*doc.root, "s");
        auto* i = gran::ui::findNode(*doc.root, "i");

        TEST_ASSERT(gran::a11y::accessibleName(*s) == "Volume");
        TEST_ASSERT(gran::a11y::accessibleName(*i) == "Login");
    }

    void testAccessDisabled()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack enabled=\"false\"><Button id=\"b\" text=\"Go\" width=\"80\" height=\"24\"/></Stack></Window>",
            400, 300);

        auto* b = gran::ui::findNode(*doc.root, "b");

        TEST_ASSERT(gran::a11y::describe(*b).enabled == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::a11y::AccessRequest request;
        request.kind = gran::a11y::Request::INVOKE;
        request.node = b;

        TEST_ASSERT(router.perform(request) == false);
    }

    void testAccessPerform()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b\" text=\"Go\" onClick=\"go\" width=\"80\" height=\"24\"/><CheckBox id=\"c\" text=\"C\" onChange=\"chk\" width=\"80\" height=\"24\"/><Slider id=\"s\" min=\"0\" max=\"10\" step=\"2\" onChange=\"vol\" width=\"100\" height=\"20\"/><Input id=\"i\" maxLength=\"5\" onChange=\"txt\" width=\"100\" height=\"20\"/><Input id=\"ro\" value=\"keep\" readOnly=\"true\" width=\"100\" height=\"20\"/></Stack></Window>",
            400, 300);

        auto* b = gran::ui::findNode(*doc.root, "b");
        auto* c = gran::ui::findNode(*doc.root, "c");
        auto* s = gran::ui::findNode(*doc.root, "s");
        auto* i = gran::ui::findNode(*doc.root, "i");
        auto* ro = gran::ui::findNode(*doc.root, "ro");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::a11y::AccessRequest request;
        request.kind = gran::a11y::Request::INVOKE;
        request.node = b;

        TEST_ASSERT(router.perform(request) == true);
        TEST_ASSERT(sink.lastAction == "go");

        request.kind = gran::a11y::Request::TOGGLE;
        request.node = c;

        TEST_ASSERT(router.perform(request) == true);
        TEST_ASSERT(c->content.checked == true);
        TEST_ASSERT(sink.lastAction == "chk");

        request.kind = gran::a11y::Request::SET_VALUE;
        request.node = s;
        request.value = 4.9f;

        TEST_ASSERT(router.perform(request) == true);
        TEST_ASSERT(std::abs(s->content.value - 4.0f) < 0.0001f);

        request.value = 4.0f;
        TEST_ASSERT(router.perform(request) == false);

        request.kind = gran::a11y::Request::SET_TEXT;
        request.node = i;
        request.text = "abc";

        TEST_ASSERT(router.perform(request) == true);
        TEST_ASSERT(i->content.text == "abc");

        request.text = "abcdefgh";
        TEST_ASSERT(router.perform(request) == false);
        TEST_ASSERT(i->content.text == "abc");

        request.text = "";
        TEST_ASSERT(router.perform(request) == true);
        TEST_ASSERT(i->content.text == "");

        request.node = ro;
        request.text = "x";
        TEST_ASSERT(router.perform(request) == false);
        TEST_ASSERT(ro->content.text == "keep");

        request.kind = gran::a11y::Request::FOCUS;
        request.node = i;

        TEST_ASSERT(router.perform(request) == true);
        TEST_ASSERT(router.focused() == i);
    }
}

namespace suites
{
    void registerAccessActionTests(test::TestRunner& runner)
    {
        runner.add("access: patterns", testAccessPatterns);
        runner.add("access: read-only", testAccessReadOnly);
        runner.add("access: names", testAccessNames);
        runner.add("access: disabled", testAccessDisabled);
        runner.add("access: perform", testAccessPerform);
        runner.add("access: modal dialog", testAccessModal);
    }
}
