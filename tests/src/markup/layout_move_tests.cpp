#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/app_driver.hpp"

namespace layout_tests
{
    void testPointerMovesButton()
    {
        using namespace gran::platform;
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Stack orientation="horizontal" spacing="12">
            <Stack id="left" width="140"><Button id="moveMe" customizable="move" text="Move"/></Stack>
            <Stack id="right" width="140" height="80" customizable="insert"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        app.setEditMode(true);
        const auto mouse = [&](EventKind kind, const char* id) {
            Event event;
            event.kind = kind;
            event.position = driver.centerOf(*app.find(id));
            driver.window(0).pushEvent(event);
            driver.settle();
        };
        mouse(EventKind::MOUSE_DOWN, "moveMe");
        mouse(EventKind::MOUSE_MOVE, "right");
        mouse(EventKind::MOUSE_UP, "right");
        TEST_ASSERT(app.find("moveMe")->parent == app.find("right"));
    }

    void testMiddleClickDoesNotCloseTabWhileEditing()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Tabs id="tabs" closable="true" onClose="closeTab">
            <Tab id="page" text="Page" customizable="move"/></Tabs></Window>)");
        int closes = 0;
        app.onAction("closeTab", [&](auto&) { ++closes; });
        gran::AppDriver driver(app);
        driver.start();
        auto& header = *app.find("tabs")->children.front()->children.front();
        app.setEditMode(true);
        driver.click(header, gran::platform::MouseButton::MIDDLE);
        TEST_ASSERT(closes == 0);
        app.setEditMode(false);
        driver.click(header, gran::platform::MouseButton::MIDDLE);
        TEST_ASSERT(closes == 1);
    }

    void testPointerReordersVerticalTabBar()
    {
        using namespace gran::platform;
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Resources><Style name="verticalBar" orientation="vertical"/></Resources>
            <Tabs id="tabs"><Tab id="a" text="A" customizable="move"/><Tab id="b" text="B" customizable="move"/></Tabs>
            </Window>)");
        auto& bar = *app.find("tabs")->children.front();
        app.setStyle(bar, "verticalBar");
        gran::AppDriver driver(app);
        driver.start();
        app.setEditMode(true);
        const auto a = driver.centerOf(*bar.children.at(0));
        const auto b = driver.centerOf(*bar.children.at(1));
        TEST_ASSERT_NEAR(a.x, b.x, 0.001f);
        TEST_ASSERT(a.y < b.y);
        Event event;
        event.kind = EventKind::MOUSE_DOWN;
        event.button = MouseButton::LEFT;
        event.position = b;
        driver.window().pushEvent(event);
        event.kind = EventKind::MOUSE_MOVE;
        event.position = {a.x, a.y - 4};
        driver.window().pushEvent(event);
        event.kind = EventKind::MOUSE_UP;
        driver.window().pushEvent(event);
        driver.settle();
        TEST_ASSERT(app.find("tabs")->children.at(1).get() == app.find("b"));
        TEST_ASSERT(app.find("tabs")->children.at(2).get() == app.find("a"));
    }
}
