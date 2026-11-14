#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/platform/host.hpp"

namespace
{
    void testDriverKeysFocusLoss()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Resources><Command id="del" key="Del" action="delAll"/></Resources><Stack padding="10" spacing="8"><Button id="ok" onClick="save">OK</Button><Input id="name"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        int saveCount = 0;
        app.onAction("save", [&saveCount](gran::ui::Node&) { ++saveCount; });

        gran::Vec2 point = driver.centerOf(*app.find("ok"));
        gran::platform::Event e;
        e.kind = gran::platform::EventKind::MOUSE_MOVE;
        e.position = point;
        driver.window(0).pushEvent(e);
        driver.settle();

        e.kind = gran::platform::EventKind::MOUSE_DOWN;
        e.button = gran::platform::MouseButton::LEFT;
        e.clicks = 1;
        driver.window(0).pushEvent(e);
        driver.settle();

        e.kind = gran::platform::EventKind::FOCUS_LOST;
        driver.window(0).pushEvent(e);
        driver.settle();

        e.kind = gran::platform::EventKind::MOUSE_UP;
        driver.window(0).pushEvent(e);
        driver.settle();

        TEST_ASSERT(saveCount == 0);

        driver.click(*app.find("ok"));
        TEST_ASSERT(saveCount == 1);
    }

    void testDriverKeysMetaMismatch()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Resources><Command id="del" key="Del" action="delAll"/></Resources><Stack padding="10" spacing="8"><Button id="ok" onClick="save">OK</Button><Input id="name"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        int delAllCount = 0;
        app.onAction("delAll", [&delAllCount](gran::ui::Node&) { ++delAllCount; });

        driver.press(gran::platform::Key::DELETE_KEY, gran::platform::MODIFIER_META);
        TEST_ASSERT(delAllCount == 0);

        driver.press(gran::platform::Key::DELETE_KEY);
        TEST_ASSERT(delAllCount == 1);
    }

    void testDriverKeysWordStep()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Resources><Command id="del" key="Del" action="delAll"/></Resources><Stack padding="10" spacing="8"><Button id="ok" onClick="save">OK</Button><Input id="name"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        int delAllCount = 0;
        app.onAction("delAll", [&delAllCount](gran::ui::Node&) { ++delAllCount; });

        driver.click(*app.find("name"));
        driver.type("one two");

        std::uint8_t word = gran::platform::MODIFIER_CONTROL;
        if (gran::platform::hostOs() == gran::platform::HostOs::MACOS) {
            word = gran::platform::MODIFIER_ALT;
        }

        driver.press(gran::platform::Key::LEFT, word);
        TEST_ASSERT(app.find("name")->content.caret == 4);

        driver.press(gran::platform::Key::BACKSPACE, word);
        TEST_ASSERT(app.find("name")->content.text == "two");

        TEST_ASSERT(delAllCount == 0);
    }
}

namespace suites
{
    void registerDriverKeyTests(test::TestRunner& runner)
    {
        runner.add("driver keys: focus loss", testDriverKeysFocusLoss);
        runner.add("driver keys: meta mismatch", testDriverKeysMetaMismatch);
        runner.add("driver keys: word step", testDriverKeysWordStep);
    }
}
