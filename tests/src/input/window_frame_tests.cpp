#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include "gran/core/gran_exception.hpp"
#include "support/layout_helpers.hpp"
#include <cmath>
#include <functional>
#include <string>

namespace
{
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

        void minimize() override
        {
            minimizeCount++;
        }

        void maximize() override
        {
            maximized = true;
            maximizeCount++;
        }

        void restore() override
        {
            maximized = false;
            restoreCount++;
        }

        void close() override
        {
            closeCount++;
        }

        bool isMaximized() const override
        {
            return maximized;
        }

        int minimizeCount = 0;
        int maximizeCount = 0;
        int restoreCount = 0;
        int closeCount = 0;
        bool maximized = false;
    };

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

    void testWindowFrameParsing()
    {
        const char* markup = R"(
            <Window titleBar="system">
                <Stack>
                    <Box id="drag" windowRole="drag"/>
                    <Box id="none" windowRole="none"/>
                    <Box id="minimize" windowRole="minimize"/>
                    <Box id="maximize" windowRole="maximize"/>
                    <Box id="close" windowRole="close"/>
                </Stack>
            </Window>
        )";

        gran::markup::UiDocument doc = support::layoutMarkup(markup, 800, 600);
        TEST_ASSERT(doc.window.titleBar == gran::platform::TitleBar::SYSTEM);

        TEST_ASSERT(support::nodeById(doc, "drag").content.windowRole == gran::platform::WindowRole::DRAG);
        TEST_ASSERT(support::nodeById(doc, "none").content.windowRole == gran::platform::WindowRole::NONE);
        TEST_ASSERT(support::nodeById(doc, "minimize").content.windowRole == gran::platform::WindowRole::MINIMIZE);
        TEST_ASSERT(support::nodeById(doc, "maximize").content.windowRole == gran::platform::WindowRole::MAXIMIZE);
        TEST_ASSERT(support::nodeById(doc, "close").content.windowRole == gran::platform::WindowRole::CLOSE);

        TEST_ASSERT_THROWS(support::layoutMarkup(R"(<Window titleBar="invalid"><Box/></Window>)", 800, 600), gran::MarkupException);
        TEST_ASSERT_THROWS(support::layoutMarkup(R"(<Window><Box windowRole="invalid"/></Window>)", 800, 600), gran::MarkupException);
        TEST_ASSERT(support::layoutMarkup("<Window><Box/></Window>", 100, 100).window.titleBar == gran::platform::TitleBar::SYSTEM);
        TEST_ASSERT(support::layoutMarkup("<Window titleBar=\"custom\"><Box/></Window>", 100, 100).window.titleBar == gran::platform::TitleBar::CUSTOM);
        auto clone = gran::ui::cloneTree(support::nodeById(doc, "close"));
        TEST_ASSERT(clone->content.windowRole == gran::platform::WindowRole::CLOSE);
    }

    void testWindowFrameRolePropagation()
    {
        const char* markup = R"(
            <Window>
                <Stack>
                    <Box id="drag" windowRole="drag" height="100">
                        <Text id="label" height="20">Title</Text>
                        <Button id="button" text="Test"/>
                        <Input id="input"/>
                    </Box>
                    <Box id="disabled" windowRole="drag" enabled="false" height="40"/>
                    <Box id="hidden" windowRole="drag" visible="false"/>
                </Stack>
            </Window>
        )";

        gran::markup::UiDocument doc = support::layoutMarkup(markup, 800, 600);

        TEST_ASSERT(support::nodeById(doc, "drag").content.windowRole == gran::platform::WindowRole::DRAG);
        TEST_ASSERT(support::nodeById(doc, "button").content.windowRole == gran::platform::WindowRole::NONE);
        TEST_ASSERT(support::nodeById(doc, "input").content.windowRole == gran::platform::WindowRole::NONE);
        TEST_ASSERT(support::nodeById(doc, "disabled").content.windowRole == gran::platform::WindowRole::DRAG);
        TEST_ASSERT(support::nodeById(doc, "hidden").content.windowRole == gran::platform::WindowRole::DRAG);

        ActionSinkStub sink;
        WindowStub window;
        TextLocatorStub locator;
        gran::input::InputRouter router(*doc.root, sink, &window, locator);
        const auto& label = support::nodeById(doc, "label").layout.frame;
        const auto& button = support::nodeById(doc, "button").layout.frame;
        const auto& input = support::nodeById(doc, "input").layout.frame;
        TEST_ASSERT(router.windowRoleAt({label.x + 5, label.y + 5}) == gran::platform::WindowRole::DRAG);
        TEST_ASSERT(router.windowRoleAt({button.x + 5, button.y + 5}) == gran::platform::WindowRole::NONE);
        TEST_ASSERT(router.windowRoleAt({input.x + 5, input.y + 5}) == gran::platform::WindowRole::NONE);
        TEST_ASSERT(router.windowRoleAt({5, 120}) == gran::platform::WindowRole::NONE);
        TEST_ASSERT(router.windowRoleAt({5, 180}) == gran::platform::WindowRole::NONE);
    }

    void testWindowFrameMouseEvents()
    {
        const char* markup = R"(
            <Window>
                <Stack>
                    <Box id="maximize" windowRole="maximize" height="40" width="80"/>
                    <Box id="minimize" windowRole="minimize" height="40" width="80"/>
                    <Box id="close" windowRole="close" height="40" width="80"/>
                </Stack>
            </Window>
        )";

        gran::markup::UiDocument doc = support::layoutMarkup(markup, 800, 600);
        WindowStub window;
        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.position = {40, 20};

        router.handle(event);
        TEST_ASSERT(!window.isMaximized());

        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);
        TEST_ASSERT(window.isMaximized());
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        router.handle(event);
        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);
        TEST_ASSERT(!window.isMaximized());
        event.position = {40, 60};
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        router.handle(event);
        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);
        event.position = {40, 100};
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        router.handle(event);
        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);

        TEST_ASSERT(window.minimizeCount == 1);
        TEST_ASSERT(window.maximizeCount == 1);
        TEST_ASSERT(window.restoreCount == 1);
        TEST_ASSERT(window.closeCount == 1);
    }

    void testWindowFrameKeyboardEvents()
    {
        const char* markup = R"(
            <Window>
                <Stack>
                    <Box id="box" windowRole="close" height="40" width="80"/>
                </Stack>
            </Window>
        )";

        gran::markup::UiDocument doc = support::layoutMarkup(markup, 800, 600);
        WindowStub window;
        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::TAB;

        router.handle(event);
        TEST_ASSERT(router.focused() == &support::nodeById(doc, "box"));

        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(window.closeCount == 1);

        event.key = gran::platform::Key::SPACE;
        router.handle(event);
        TEST_ASSERT(window.closeCount == 2);

        TEST_ASSERT(gran::a11y::roleOf(support::nodeById(doc, "box")) == gran::a11y::Role::BUTTON);
        TEST_ASSERT(gran::a11y::patternsOf(support::nodeById(doc, "box")) & gran::a11y::PATTERN_INVOKE);
        gran::a11y::AccessRequest request;
        request.kind = gran::a11y::Request::INVOKE;
        request.node = gran::ui::findNode(*doc.root, "box");
        TEST_ASSERT(router.perform(request));
        TEST_ASSERT(window.closeCount == 3);
    }

    void testWindowFrameMaximizeState()
    {
        const char* markup = R"(
            <Window>
                <Stack id="root">
                    <Box id="child"/>
                </Stack>
            </Window>
        )";

        gran::markup::UiDocument doc = support::layoutMarkup(markup, 800, 600);
        gran::ui::Node* root = gran::ui::findNode(*doc.root, "root");

        TEST_ASSERT(!(root->stateMask & gran::style::stateBit(gran::style::State::MAXIMIZED)));

        gran::ui::setWindowMaximized(*root, true);
        TEST_ASSERT(root->stateMask & gran::style::stateBit(gran::style::State::MAXIMIZED));
        TEST_ASSERT(support::nodeById(doc, "child").stateMask & gran::style::stateBit(gran::style::State::MAXIMIZED));

        gran::ui::setWindowMaximized(*root, false);
        TEST_ASSERT(!(root->stateMask & gran::style::stateBit(gran::style::State::MAXIMIZED)));
        TEST_ASSERT(!(support::nodeById(doc, "child").stateMask & gran::style::stateBit(gran::style::State::MAXIMIZED)));
    }

    // Рамка использует геометрию обычного ввода и не обходит модальность диалога.
    // The frame uses regular input geometry and cannot bypass dialog modality.
    void testWindowFrameHitGeometry()
    {
        auto doc = support::layoutMarkup(R"XML(<Window><Canvas>
            <Canvas width="100" height="100" overflow="hidden"><Box left="80" top="20" width="40" height="40" windowRole="drag"/></Canvas>
            <Box left="200" width="80" height="40" transform="translate(30, 20)" windowRole="drag"/>
            <Dialog id="modal" visible="false"><Text>Modal</Text></Dialog>
            </Canvas></Window>)XML", 400, 300);
        ActionSinkStub sink;
        WindowStub window;
        TextLocatorStub locator;
        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        TEST_ASSERT(router.windowRoleAt({90, 30}) == gran::platform::WindowRole::DRAG);
        TEST_ASSERT(router.windowRoleAt({110, 30}) == gran::platform::WindowRole::NONE);
        TEST_ASSERT(router.windowRoleAt({235, 25}) == gran::platform::WindowRole::DRAG);
        TEST_ASSERT(router.windowRoleAt({205, 5}) == gran::platform::WindowRole::NONE);
        auto* dialog = gran::ui::findNode(*doc.root, "modal");
        router.openPopup(*dialog, gran::Rect{}, false);
        TEST_ASSERT(router.windowRoleAt({90, 30}) == gran::platform::WindowRole::NONE);
        router.closePopup(*dialog);
        TEST_ASSERT(router.windowRoleAt({90, 30}) == gran::platform::WindowRole::DRAG);
    }

    void addWindowFrameTests(test::TestRunner& runner)
    {
        runner.add("testWindowFrameParsing", testWindowFrameParsing);
        runner.add("testWindowFrameRolePropagation", testWindowFrameRolePropagation);
        runner.add("testWindowFrameMouseEvents", testWindowFrameMouseEvents);
        runner.add("testWindowFrameKeyboardEvents", testWindowFrameKeyboardEvents);
        runner.add("testWindowFrameMaximizeState", testWindowFrameMaximizeState);
        runner.add("window frame: clipping, transform and modality", testWindowFrameHitGeometry);
    }
}

namespace suites
{
    void registerWindowFrameTests(test::TestRunner& runner)
    {
        addWindowFrameTests(runner);
    }
}
