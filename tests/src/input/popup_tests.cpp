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

    void testPopupVisibilityAndLayout()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b\" text=\"x\" width=\"80\" height=\"30\"/><Text id=\"t\">hello world</Text><Popup id=\"p\"><MenuItem id=\"i1\" text=\"One\" onClick=\"one\"/><MenuItem id=\"i2\" text=\"Two\"/></Popup></Stack></Window>",
            400, 300);

        auto* b = gran::ui::findNode(*doc.root, "b");
        auto* t = gran::ui::findNode(*doc.root, "t");
        auto* p = gran::ui::findNode(*doc.root, "p");

        TEST_ASSERT(p->visible == false);
        TEST_ASSERT(std::abs(b->layout.frame.y - 0.0f) < 0.01f);
        TEST_ASSERT(std::abs(t->layout.frame.y - 30.0f) < 0.01f);

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.openPopup(*p, gran::Rect{100, 50, 80, 30}, false);
        support::FixedMeasurer measurer;
        gran::layout::layoutTree(*doc.root, gran::Size{400, 300}, measurer);
        TEST_ASSERT(p->visible == true);
        TEST_ASSERT(std::abs(p->layout.frame.x - 100.0f) < 0.01f);
        TEST_ASSERT(std::abs(p->layout.frame.y - 84.0f) < 0.01f);

        router.openPopup(*p, gran::Rect{390, 290, 0, 0}, false);
        gran::layout::layoutTree(*doc.root, gran::Size{400, 300}, measurer);
        TEST_ASSERT(p->visible == true);
        // У якоря-точки слой отражается внутрь окна / At a point anchor the layer is mirrored into the window
        TEST_ASSERT(std::abs(p->layout.frame.x - (390.0f - p->layout.frame.width)) < 0.01f);
        TEST_ASSERT(std::abs(p->layout.frame.y - (290.0f - p->layout.frame.height)) < 0.01f);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::ESCAPE;
        router.handle(event);
        TEST_ASSERT(p->visible == false);
    }

    void testPopupNavigation()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b\" text=\"x\" width=\"80\" height=\"30\"/><Text id=\"t\">hello world</Text><Popup id=\"p\"><MenuItem id=\"i1\" text=\"One\" onClick=\"one\"/><MenuItem id=\"i2\" text=\"Two\"/></Popup></Stack></Window>",
            400, 300);

        auto* p = gran::ui::findNode(*doc.root, "p");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.openPopup(*p, gran::Rect{100, 50, 80, 30}, false);
        TEST_ASSERT(router.focused() == nullptr);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::KEY_DOWN;
        event.key = gran::platform::Key::DOWN;
        router.handle(event);
        TEST_ASSERT(router.focused() == gran::ui::findNode(*p, "i1"));

        router.handle(event);
        TEST_ASSERT(router.focused() == gran::ui::findNode(*p, "i2"));

        event.key = gran::platform::Key::UP;
        router.handle(event);
        TEST_ASSERT(router.focused() == gran::ui::findNode(*p, "i1"));

        event.key = gran::platform::Key::ENTER;
        router.handle(event);
        TEST_ASSERT(sink.actionCount == 1);
        TEST_ASSERT(sink.lastAction == "one");
        TEST_ASSERT(p->visible == false);
    }

    void testPopupClickOutside()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b\" text=\"x\" width=\"80\" height=\"30\"/><Text id=\"t\">hello world</Text><Popup id=\"p\"><MenuItem id=\"i1\" text=\"One\" onClick=\"one\"/><MenuItem id=\"i2\" text=\"Two\"/></Popup></Stack></Window>",
            400, 300);

        auto* p = gran::ui::findNode(*doc.root, "p");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        router.openPopup(*p, gran::Rect{100, 50, 80, 30}, false);
        support::FixedMeasurer measurer;
        gran::layout::layoutTree(*doc.root, gran::Size{400, 300}, measurer);
        TEST_ASSERT(p->visible == true);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::LEFT;
        event.position = {390, 10};
        router.handle(event);
        TEST_ASSERT(p->visible == false);
    }

    void testContextMenu()
    {
        auto doc = support::layoutMarkup(
            "<Window><Stack><Button id=\"b\" text=\"x\" width=\"80\" height=\"30\"/><Text id=\"t\">hello world</Text><Popup id=\"p\"><MenuItem id=\"i1\" text=\"One\" onClick=\"one\"/><MenuItem id=\"i2\" text=\"Two\"/></Popup></Stack></Window>",
            400, 300);

        auto* t = gran::ui::findNode(*doc.root, "t");

        ActionSinkStub sink;
        TextLocatorStub locator;
        gran::platform::HeadlessWindow window;

        gran::input::InputRouter router(*doc.root, sink, &window, locator);

        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_DOWN;
        event.button = gran::platform::MouseButton::RIGHT;
        event.position = {t->layout.frame.x + t->layout.frame.width / 2, t->layout.frame.y + t->layout.frame.height / 2};
        router.handle(event);

        auto* menu = doc.root->children.back().get();
        TEST_ASSERT(menu->visible == true);
        TEST_ASSERT(menu->children.size() == 5);

        auto* undoItem = menu->children[0].get();
        auto* redoItem = menu->children[1].get();
        auto* item1 = menu->children[2].get();
        auto* item2 = menu->children[3].get();
        auto* item3 = menu->children[4].get();

        TEST_ASSERT(undoItem->content.text == u8"Отменить");
        TEST_ASSERT(redoItem->content.text == u8"Повторить");
        // У Text нет истории правок / A Text has no edit history
        TEST_ASSERT(!undoItem->enabled && !redoItem->enabled);
        TEST_ASSERT(item1->content.text == u8"Копировать");
        TEST_ASSERT(item2->content.text == u8"Вставить");
        TEST_ASSERT(item3->content.text == u8"Выделить всё");
    }
}

namespace suites
{
    void registerPopupTests(test::TestRunner& runner)
    {
        runner.add("popup: visibility and layout", testPopupVisibilityAndLayout);
        runner.add("popup: navigation", testPopupNavigation);
        runner.add("popup: click outside", testPopupClickOutside);
        runner.add("popup: context menu", testContextMenu);
    }
}
