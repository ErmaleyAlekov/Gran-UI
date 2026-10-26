#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
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

        void wake() noexcept override
        {
        }

        std::string clipboardText_;
    };

    void testTextAreaKindAndContent()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><TextArea id=\"a\" rows=\"3\" submitKey=\"enter\">\nстрока1\nстрока2</TextArea><TextArea id=\"d\"/></Stack></Window>",
            400, 300);

        auto* nodeA = gran::ui::findNode(*doc.root, "a");
        TEST_ASSERT(nodeA->kind == gran::ui::ElementKind::TEXT_AREA);
        TEST_ASSERT(nodeA->content.text == u8"строка1\nстрока2");
        TEST_ASSERT(nodeA->content.rows == 3);
        TEST_ASSERT(nodeA->content.submitKey == gran::ui::SubmitKey::ENTER);

        auto* nodeD = gran::ui::findNode(*doc.root, "d");
        TEST_ASSERT(nodeD->content.rows == 2);
        TEST_ASSERT(nodeD->content.submitKey == gran::ui::SubmitKey::CONTROL_ENTER);
    }

    void testTextAreaLayout()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><TextArea id=\"e\" rows=\"3\"/></Stack></Window>",
            400, 300);

        auto* nodeE = gran::ui::findNode(*doc.root, "e");
        TEST_ASSERT(std::abs(nodeE->layout.frame.width - 400.0f) < 0.01f);
        TEST_ASSERT(std::abs(nodeE->layout.frame.height - 76.8f) < 0.01f);
    }

    void testTextAreaMaxHeight()
    {
        std::string longText(200, 'x');
        std::string markup = "<Window><Stack><TextArea id=\"m\" rows=\"1\" maxHeight=\"50\" value=\"" + longText
            + "\"/></Stack></Window>";
        auto doc = support::layoutMarkup(markup.c_str(), 400, 300);

        auto* nodeM = gran::ui::findNode(*doc.root, "m");
        TEST_ASSERT(std::abs(nodeM->layout.frame.height - 50.0f) < 0.01f);
        TEST_ASSERT(nodeM->layout.contentSize.height > 50.0f);
    }

    void testTextAreaEnterAndSubmit()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><TextArea id=\"c\" onSubmit=\"send\"/><TextArea id=\"s\" submitKey=\"enter\" onSubmit=\"send\"/><Input id=\"i\"/></Stack></Window>",
            400, 300);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        auto* nodeC = gran::ui::findNode(*doc.root, "c");
        auto* nodeS = gran::ui::findNode(*doc.root, "s");
        auto* nodeI = gran::ui::findNode(*doc.root, "i");

        router.focus(nodeC);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::CHARACTER;
        event.character = 'a';
        router.handle(event);
        event.character = 'b';
        router.handle(event);
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(nodeC->content.text == u8"ab\n");
        TEST_ASSERT(sink.actionCount == 0);

        event.modifiers = gran::platform::MODIFIER_CONTROL;
        router.handle(event);
        TEST_ASSERT(sink.actionCount == 1);
        TEST_ASSERT(sink.lastAction == "send");
        TEST_ASSERT(nodeC->content.text == u8"ab\n");

        router.focus(nodeS);
        event.kind = gran::platform::EventKind::CHARACTER;
        event.modifiers = 0;
        event.character = 'x';
        router.handle(event);
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(sink.actionCount == 2);
        TEST_ASSERT(sink.lastAction == "send");
        TEST_ASSERT(nodeS->content.text == u8"x");

        event.modifiers = gran::platform::MODIFIER_SHIFT;
        router.handle(event);
        TEST_ASSERT(nodeS->content.text == u8"x\n");
        TEST_ASSERT(sink.actionCount == 2);

        // Вставка из буфера: многострочное поле — \n, однострочное — пробелы / A paste: a multi-line field gets \n, a single-line one — spaces
        nodeC->content.text.clear();
        nodeC->content.caret = 0;
        nodeC->content.anchor = 0;
        window.clipboardText_ = "a\r\nb\rc";
        router.focus(nodeC);
        event.key = gran::platform::Key::V;
        event.modifiers = gran::platform::MODIFIER_CONTROL;
        router.handle(event);
        TEST_ASSERT(nodeC->content.text == u8"a\nb\nc");

        router.focus(nodeI);
        router.handle(event);
        TEST_ASSERT(nodeI->content.text == u8"a  b c");
        (void)nodeS;
    }
}

namespace suites
{
    void registerTextAreaTests(test::TestRunner& runner)
    {
        runner.add("textarea: kind and content", testTextAreaKindAndContent);
        runner.add("textarea: layout", testTextAreaLayout);
        runner.add("textarea: max height", testTextAreaMaxHeight);
        runner.add("textarea: enter and submit", testTextAreaEnterAndSubmit);
    }
}
