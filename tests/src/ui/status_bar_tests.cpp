#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include "support/layout_helpers.hpp"
#include "gran/a11y/access_tree.hpp"
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
                <Text id="body" grow="1">Документ</Text>
                <StatusBar id="bar" sizeGrip="true">
                    <StatusSection id="main" grow="1">
                        <Text id="msg">Готово</Text>
                        <Text id="extra">!</Text>
                    </StatusSection>
                    <StatusSection id="pos">
                        <Text>Строка 1</Text>
                    </StatusSection>
                </StatusBar>
            </Stack>
        </Window>
    )";

    void testGrip()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* bar = gran::ui::findNode(*doc.root, "bar");

        TEST_ASSERT(bar->children.back()->kind == gran::ui::ElementKind::SIZE_GRIP);
        TEST_ASSERT(bar->children.back()->content.windowRole == gran::platform::WindowRole::RESIZE);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        auto* grip = bar->children.back().get();
        auto center = gran::Vec2{grip->layout.frame.x + grip->layout.frame.width / 2, grip->layout.frame.y + grip->layout.frame.height / 2};
        TEST_ASSERT(router.windowRoleAt(center) == gran::platform::WindowRole::RESIZE);

        auto* pos = gran::ui::findNode(*doc.root, "pos");
        TEST_ASSERT(router.windowRoleAt({pos->layout.frame.x + pos->layout.frame.width / 2, pos->layout.frame.y + pos->layout.frame.height / 2}) == gran::platform::WindowRole::NONE);
    }

    void testStatusText()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* bar = gran::ui::findNode(*doc.root, "bar");
        auto* main = gran::ui::findNode(*doc.root, "main");

        TEST_ASSERT(gran::ui::statusText(*main) == u8"Готово !");
        TEST_ASSERT(gran::a11y::roleOf(*bar) == gran::a11y::Role::STATUS_BAR);
        TEST_ASSERT(gran::a11y::roleOf(*main) == gran::a11y::Role::TEXT);
        TEST_ASSERT(gran::a11y::accessibleName(*main) == u8"Готово !");
    }

    void testChanges()
    {
        auto doc = support::layoutMarkup(MARKUP, 600, 400);
        auto* main = gran::ui::findNode(*doc.root, "main");
        auto* msg = gran::ui::findNode(*doc.root, "msg");
        auto* pos = gran::ui::findNode(*doc.root, "pos");

        std::vector<gran::ui::Node*> changed;
        gran::ui::collectStatusChanges(*doc.root, changed);
        TEST_ASSERT(changed.empty());

        msg->content.text = u8"Сохранено";
        gran::ui::collectStatusChanges(*doc.root, changed);
        TEST_ASSERT(changed.size() == 1 && changed[0] == main);
        changed.clear();

        gran::ui::collectStatusChanges(*doc.root, changed);
        TEST_ASSERT(changed.empty());

        pos->visible = false;
        pos->children[0]->content.text = "x";
        gran::ui::collectStatusChanges(*doc.root, changed);
        TEST_ASSERT(changed.empty());
    }
}

namespace suites
{
    void registerStatusBarTests(test::TestRunner& runner)
    {
        runner.add("status bar: grip", testGrip);
        runner.add("status bar: status text", testStatusText);
        runner.add("status bar: changes", testChanges);
    }
}
