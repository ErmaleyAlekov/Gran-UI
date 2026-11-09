#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/input_router.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include "support/layout_helpers.hpp"
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
            <Stack>
                <ToolBar id="bar">
                    <Button id="a" text="Один" onClick="one"/>
                    <Separator id="s"/>
                    <ToolGroup id="g">
                        <Button id="b" text="Два" onClick="two"/>
                        <Button id="c" text="Три" onClick="three"/>
                    </ToolGroup>
                    <Button id="d" tooltip="Четыре" onClick="four">
                        <Icon data="M0 0h10v10z" width="16" height="16"/>
                    </Button>
                    <Button id="e" text="Пять" overflowMode="always" onClick="five"/>
                </ToolBar>
            </Stack>
        </Window>
    )";

    void testMirrors()
    {
        auto doc = support::layoutMarkup(MARKUP, 800, 200);
        auto* bar = gran::ui::findNode(*doc.root, "bar");

        auto* more = gran::ui::toolBarMore(*bar);
        TEST_ASSERT(more != nullptr);

        auto* menu = more->children[0].get();
        TEST_ASSERT(menu->children.size() == 6);

        TEST_ASSERT(menu->children[0]->kind == gran::ui::ElementKind::MENU_ITEM);
        TEST_ASSERT(menu->children[1]->kind == gran::ui::ElementKind::SEPARATOR);
        TEST_ASSERT(menu->children[2]->kind == gran::ui::ElementKind::MENU_ITEM);
        TEST_ASSERT(menu->children[3]->kind == gran::ui::ElementKind::MENU_ITEM);
        TEST_ASSERT(menu->children[4]->kind == gran::ui::ElementKind::MENU_ITEM);
        TEST_ASSERT(menu->children[5]->kind == gran::ui::ElementKind::MENU_ITEM);

        TEST_ASSERT(menu->children[0]->content.text == u8"Один");
        TEST_ASSERT(menu->children[1]->content.text.empty());
        TEST_ASSERT(menu->children[2]->content.text == u8"Два");
        TEST_ASSERT(menu->children[3]->content.text == u8"Три");
        TEST_ASSERT(menu->children[4]->content.text == u8"Четыре");
        TEST_ASSERT(menu->children[5]->content.text == u8"Пять");

        TEST_ASSERT(menu->children[0]->content.onClick == "one");
        TEST_ASSERT(menu->children[5]->content.onClick == "five");
    }

    void testAlwaysOverflow()
    {
        auto doc = support::layoutMarkup(MARKUP, 800, 200);
        auto* bar = gran::ui::findNode(*doc.root, "bar");
        auto* e = gran::ui::findNode(*doc.root, "e");
        auto* a = gran::ui::findNode(*doc.root, "a");
        auto* b = gran::ui::findNode(*doc.root, "b");
        auto* c = gran::ui::findNode(*doc.root, "c");
        auto* d = gran::ui::findNode(*doc.root, "d");

        TEST_ASSERT(e->content.overflowed);
        TEST_ASSERT(!e->visible);

        TEST_ASSERT(a->visible && !a->content.overflowed);
        TEST_ASSERT(b->visible && !b->content.overflowed);
        TEST_ASSERT(c->visible && !c->content.overflowed);
        TEST_ASSERT(d->visible && !d->content.overflowed);

        auto* more = gran::ui::toolBarMore(*bar);
        TEST_ASSERT(more->visible);

        auto* menu = more->children[0].get();
        TEST_ASSERT(menu->children[5]->visible);
        TEST_ASSERT(!menu->children[0]->visible);
    }

    void testNarrow()
    {
        auto doc = support::layoutMarkup(MARKUP, 120, 200);
        auto* bar = gran::ui::findNode(*doc.root, "bar");
        auto* a = gran::ui::findNode(*doc.root, "a");
        auto* d = gran::ui::findNode(*doc.root, "d");
        auto* g = gran::ui::findNode(*doc.root, "g");
        auto* e = gran::ui::findNode(*doc.root, "e");

        TEST_ASSERT(a->visible);
        TEST_ASSERT(d->content.overflowed);
        TEST_ASSERT(g->content.overflowed && !g->visible);

        auto* more = gran::ui::toolBarMore(*bar);
        TEST_ASSERT(more->visible);

        auto* menu = more->children[0].get();
        TEST_ASSERT(menu->children[2]->visible);
        TEST_ASSERT(menu->children[3]->visible);
        TEST_ASSERT(menu->children[4]->visible);
        TEST_ASSERT(menu->children[5]->visible);
        TEST_ASSERT(!menu->children[0]->visible);

        support::FixedMeasurer measurer;
        gran::layout::layoutTree(*doc.root, gran::Size{800, 200}, measurer);

        TEST_ASSERT(d->visible && !d->content.overflowed);
        TEST_ASSERT(g->visible && !g->content.overflowed);
        TEST_ASSERT(e->content.overflowed);

        TEST_ASSERT(!menu->children[4]->visible);
        TEST_ASSERT(menu->children[5]->visible);
        TEST_ASSERT(!menu->children[0]->visible);
    }

    void testMoreMenu()
    {
        auto doc = support::layoutMarkup(MARKUP, 120, 200);
        auto* bar = gran::ui::findNode(*doc.root, "bar");

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        auto* more = gran::ui::toolBarMore(*bar);
        router.focus(more);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(more->children[0]->visible);

        // Меню, открытое с клавиатуры, сразу ставит фокус на первый видимый пункт
        // A menu opened from the keyboard puts the focus on the first visible item at once
        auto* menu = more->children[0].get();
        gran::ui::Node* focusedItem = nullptr;
        for (auto& child : menu->children)
        {
            if (child->visible && child->kind == gran::ui::ElementKind::MENU_ITEM)
            {
                focusedItem = child.get();
                break;
            }
        }
        TEST_ASSERT(router.focused() == focusedItem);

        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(sink.lastAction == focusedItem->content.onClick);
        TEST_ASSERT(!menu->visible);
    }
}

namespace suites
{
    void registerToolBarTests(test::TestRunner& runner)
    {
        runner.add("toolbar: mirrors", testMirrors);
        runner.add("toolbar: always overflow", testAlwaysOverflow);
        runner.add("toolbar: narrow", testNarrow);
        runner.add("toolbar: more menu", testMoreMenu);
    }
}
