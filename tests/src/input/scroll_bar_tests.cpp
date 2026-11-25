#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include "gran/ui/node.hpp"
#include <cmath>

namespace
{
    void testLargeScrollThumb()
    {
        constexpr float extent = 1e20f;
        gran::ui::Node scroll;
        scroll.kind = gran::ui::ElementKind::SCROLL;
        scroll.layout.frame = {0, 0, extent, extent};
        scroll.layout.contentSize = {2 * extent, 2 * extent};
        scroll.layout.scrollOffset = {extent / 2, extent / 2};
        for (const bool vertical : {false, true})
        {
            gran::ui::ScrollBar bar;
            TEST_ASSERT(gran::ui::scrollBar(scroll, vertical, bar));
            const float length = vertical ? bar.thumb.height : bar.thumb.width;
            const float place = vertical ? bar.thumb.y : bar.thumb.x;
            TEST_ASSERT(std::isfinite(length) && std::isfinite(bar.travel));
            TEST_ASSERT_NEAR(length / extent, 0.5f);
            TEST_ASSERT_NEAR(bar.travel / extent, 0.5f);
            TEST_ASSERT_NEAR(place / extent, 0.25f);
        }
        scroll.layout.frame = {0, 0, 10, 12};
        gran::ui::ScrollBar small;
        TEST_ASSERT(gran::ui::scrollBar(scroll, false, small));
        TEST_ASSERT(small.thumb.width == 10 && small.travel == 0);
        TEST_ASSERT(gran::ui::scrollBar(scroll, true, small));
        TEST_ASSERT(small.thumb.height == 12 && small.travel == 0);
    }

    // Окно 300×200, прокрутка во всё окно, содержимое высотой 1000: бегунок 40, путь 160, предел 800
    // A 300×200 window, a scroll over the whole window, 1000 high content: the thumb is 40, the travel 160, the limit 800
    const char* MARKUP = R"(<Window width="300" height="200"><Scroll id="s" grow="1"><Stack id="content" height="1000"><Button id="under" onClick="hit">под полосой</Button></Stack></Scroll></Window>)";

    // Событие мыши левой кнопкой в точке окна / A left button mouse event at a window point
    gran::platform::Event mouse(gran::platform::EventKind kind, float x, float y)
    {
        gran::platform::Event event;

        event.kind = kind;
        event.position = gran::Vec2{x, y};
        event.button = gran::platform::MouseButton::LEFT;
        event.clicks = 1;
        return event;
    }

    // Подать событие и дождаться кадра / Push an event and wait for a frame
    void send(gran::AppDriver& driver, const gran::platform::Event& event)
    {
        driver.window(0).pushEvent(event);
        driver.settle();
    }

    // Т-58: бегунок хватается мышью и идёт за указателем, отпускание — не щелчок по содержимому
    // T-58: the thumb is grabbed by the mouse and follows the pointer, the release is not a click on the content
    void testScrollBarThumbDrag()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        int hits = 0;
        app.onAction("hit", [&hits](gran::ui::Node&) { ++hits; });
        driver.start();
        gran::ui::Node* scroll = app.find("s");
        TEST_ASSERT(scroll != nullptr);
        gran::ui::ScrollBar bar;
        TEST_ASSERT(gran::ui::scrollBar(*scroll, true, bar));
        TEST_ASSERT_NEAR(bar.thumb.height, 40.0f, 0.5f);
        float x = bar.thumb.x + bar.thumb.width * 0.5f;
        float y = bar.thumb.y + 20.0f;

        send(driver, mouse(gran::platform::EventKind::MOUSE_MOVE, x, y));
        send(driver, mouse(gran::platform::EventKind::MOUSE_DOWN, x, y));
        send(driver, mouse(gran::platform::EventKind::MOUSE_MOVE, x, y + 80.0f));
        TEST_ASSERT_NEAR(scroll->layout.scrollOffset.y, 400.0f, 1.0f);
        // Мышь ушла за окно вниз — бегунок упирается в конец / The mouse left the window downward — the thumb rests at the end
        send(driver, mouse(gran::platform::EventKind::MOUSE_MOVE, x, y + 1000.0f));
        TEST_ASSERT_NEAR(scroll->layout.scrollOffset.y, 800.0f, 1.0f);
        send(driver, mouse(gran::platform::EventKind::MOUSE_MOVE, x, y));
        send(driver, mouse(gran::platform::EventKind::MOUSE_UP, x, y));
        TEST_ASSERT_NEAR(scroll->layout.scrollOffset.y, 0.0f, 1.0f);
        TEST_ASSERT(hits == 0);
    }

    // Нажатие на полосу мимо бегунка листает на видимую часть / A press on the bar beside the thumb pages by the visible part
    void testScrollBarTrackPaging()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* scroll = app.find("s");
        TEST_ASSERT(scroll != nullptr);
        gran::ui::ScrollBar bar;
        TEST_ASSERT(gran::ui::scrollBar(*scroll, true, bar));
        float x = bar.track.x + bar.track.width * 0.5f;
        float below = bar.track.y + bar.track.height - 5.0f;

        send(driver, mouse(gran::platform::EventKind::MOUSE_DOWN, x, below));
        send(driver, mouse(gran::platform::EventKind::MOUSE_UP, x, below));
        TEST_ASSERT_NEAR(scroll->layout.scrollOffset.y, bar.view, 1.0f);
        TEST_ASSERT(gran::ui::scrollBar(*scroll, true, bar));
        float above = bar.thumb.y - 5.0f;
        send(driver, mouse(gran::platform::EventKind::MOUSE_DOWN, x, above));
        send(driver, mouse(gran::platform::EventKind::MOUSE_UP, x, above));
        TEST_ASSERT_NEAR(scroll->layout.scrollOffset.y, 0.0f, 1.0f);
    }
}

namespace suites
{
    void registerScrollBarTests(test::TestRunner& runner)
    {
        runner.add("scroll bar: keeps large finite ratios representable", testLargeScrollThumb);
        runner.add("scroll bar: thumb drag", testScrollBarThumbDrag);
        runner.add("scroll bar: track paging", testScrollBarTrackPaging);
    }
}
