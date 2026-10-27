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

    void testDialogVisibilityAndFocus()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b\" text=\"x\" width=\"80\" height=\"30\"/><Dialog id=\"d\"><Input id=\"a\"/><Button id=\"ok\" text=\"OK\"/></Dialog></Stack></Window>",
            400, 300);

        auto* b = gran::ui::findNode(*doc.root, "b");
        auto* d = gran::ui::findNode(*doc.root, "d");
        auto* a = gran::ui::findNode(*doc.root, "a");
        auto* ok = gran::ui::findNode(*doc.root, "ok");

        TEST_ASSERT(d->visible == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.focus(b);
        router.openPopup(*d, gran::Rect{}, false);
        support::FixedMeasurer measurer;
        gran::layout::layoutTree(*doc.root, gran::Size{400, 300}, measurer);

        TEST_ASSERT(d->visible == true);
        TEST_ASSERT(router.focused() == a);

        TEST_ASSERT(std::abs(d->layout.frame.x - (400 - d->layout.frame.width) / 2) < 1);
        TEST_ASSERT(std::abs(d->layout.frame.y - (300 - d->layout.frame.height) / 2) < 1);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::TAB;
        router.handle(event);
        TEST_ASSERT(router.focused() == ok);

        router.handle(event);
        TEST_ASSERT(router.focused() == a);

        event.key = gran::platform::Key::ESCAPE;
        router.handle(event);
        TEST_ASSERT(d->visible == false);
        TEST_ASSERT(router.focused() == b);
    }

    void testDialogUnclosable()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b\" text=\"x\" width=\"80\" height=\"30\"/><Dialog id=\"d\" closable=\"false\"><Input id=\"a\"/><Button id=\"ok\" text=\"OK\"/></Dialog></Stack></Window>",
            400, 300);

        auto* b = gran::ui::findNode(*doc.root, "b");
        auto* d = gran::ui::findNode(*doc.root, "d");

        TEST_ASSERT(d->visible == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.focus(b);
        router.openPopup(*d, gran::Rect{}, false);
        support::FixedMeasurer measurer;
        gran::layout::layoutTree(*doc.root, gran::Size{400, 300}, measurer);

        TEST_ASSERT(d->visible == true);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ESCAPE;
        router.handle(event);
        TEST_ASSERT(d->visible == true);
    }

    void testDialogModality()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b\" text=\"x\" width=\"80\" height=\"30\" onClick=\"hit\"/><Dialog id=\"d\"><Button id=\"ok\" text=\"OK\"/></Dialog></Stack></Window>",
            400, 300);

        auto* d = gran::ui::findNode(*doc.root, "d");

        TEST_ASSERT(d->visible == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.openPopup(*d, gran::Rect{}, false);
        support::FixedMeasurer measurer;
        gran::layout::layoutTree(*doc.root, gran::Size{400, 300}, measurer);

        TEST_ASSERT(d->visible == true);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {10, 10};
        router.handle(event);
        event.kind = gran::platform::EventKind::MOUSE_UP;
        router.handle(event);

        TEST_ASSERT(sink.actionCount == 0);
        TEST_ASSERT(d->visible == true);
    }

    void testSubmenuNavigation()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Popup id=\"p\"><MenuItem id=\"i1\" text=\"One\"/><MenuItem id=\"sub\" text=\"More\"><Popup id=\"sp\"><MenuItem id=\"s1\" text=\"Deep\" onClick=\"deep\"/></Popup></MenuItem></Popup></Stack></Window>",
            400, 300);

        auto* p = gran::ui::findNode(*doc.root, "p");
        auto* sub = gran::ui::findNode(*doc.root, "sub");
        auto* sp = gran::ui::findNode(*doc.root, "sp");
        auto* s1 = gran::ui::findNode(*doc.root, "s1");

        TEST_ASSERT(sp->visible == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.openPopup(*p, gran::Rect{20, 20, 80, 30}, false);
        support::FixedMeasurer measurer;
        gran::layout::layoutTree(*doc.root, gran::Size{400, 300}, measurer);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::DOWN;
        router.handle(event);
        router.handle(event);
        TEST_ASSERT(router.focused() == sub);

        event.key = gran::platform::Key::RIGHT;
        router.handle(event);
        TEST_ASSERT(sp->visible == true);
        TEST_ASSERT(router.focused() == s1);
        gran::layout::layoutTree(*doc.root, gran::Size{400, 300}, measurer);
        TEST_ASSERT(sp->layout.frame.x >= p->layout.frame.x + p->layout.frame.width - 0.5f);

        event.key = gran::platform::Key::LEFT;
        router.handle(event);
        TEST_ASSERT(sp->visible == false);
        TEST_ASSERT(router.focused() == sub);

        event.key = gran::platform::Key::RIGHT;
        router.handle(event);
        TEST_ASSERT(sp->visible == true);
        TEST_ASSERT(router.focused() == s1);

        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(sink.lastAction == "deep");
        TEST_ASSERT(p->visible == false);
        TEST_ASSERT(sp->visible == false);
    }

    void testMenuButtonNavigation()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b\" text=\"x\" popup=\"p\"/><Popup id=\"p\"><MenuItem id=\"i1\" text=\"One\"/><MenuItem id=\"i2\" text=\"Two\"/></Popup></Stack></Window>",
            400, 300);

        auto* b = gran::ui::findNode(*doc.root, "b");
        auto* p = gran::ui::findNode(*doc.root, "p");
        auto* i1 = gran::ui::findNode(*doc.root, "i1");

        TEST_ASSERT(p->visible == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.focus(b);
        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::DOWN;
        router.handle(event);
        TEST_ASSERT(p->visible == true);
        TEST_ASSERT(router.focused() == i1);

        event.key = gran::platform::Key::ESCAPE;
        router.handle(event);
        TEST_ASSERT(p->visible == false);
        TEST_ASSERT(router.focused() == b);

        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(p->visible == true);
        TEST_ASSERT(router.focused() == i1);
    }

    void testComboBoxBehavior()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><ComboBox id=\"c\" value=\"m\" onChange=\"changed\"><MenuItem id=\"s\" value=\"s\" text=\"Small\"/><MenuItem id=\"m\" value=\"m\" text=\"Medium\"/></ComboBox></Stack></Window>",
            400, 300);

        auto* c = gran::ui::findNode(*doc.root, "c");
        auto* s = gran::ui::findNode(*doc.root, "s");
        auto* m = gran::ui::findNode(*doc.root, "m");

        TEST_ASSERT(c->content.text == "Medium");
        TEST_ASSERT(c->content.option == "m");

        auto* list = c->children.back().get();
        TEST_ASSERT(list->visible == false);

        ActionSinkStub sink;
        TextLocatorStub locator;
        WindowStub window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.focus(c);
        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(list->visible == true);
        TEST_ASSERT(router.focused() == m);

        event.key = gran::platform::Key::UP;
        router.handle(event);
        TEST_ASSERT(router.focused() == s);

        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(c->content.option == "s");
        TEST_ASSERT(c->content.text == "Small");
        TEST_ASSERT(sink.lastAction == "changed");
        TEST_ASSERT(sink.actionCount == 1);
        TEST_ASSERT(list->visible == false);
        TEST_ASSERT(router.focused() == c);

        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(list->visible == true);

        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(sink.actionCount == 1);
        TEST_ASSERT(list->visible == false);
        TEST_ASSERT(router.focused() == c);
    }
}

namespace suites
{
    void registerDialogTests(test::TestRunner& runner)
    {
        runner.add("dialog: visibility and focus", testDialogVisibilityAndFocus);
        runner.add("dialog: unclosable", testDialogUnclosable);
        runner.add("dialog: modality", testDialogModality);
        runner.add("dialog: submenu navigation", testSubmenuNavigation);
        runner.add("dialog: menu button navigation", testMenuButtonNavigation);
        runner.add("dialog: combobox behavior", testComboBoxBehavior);
    }
}
