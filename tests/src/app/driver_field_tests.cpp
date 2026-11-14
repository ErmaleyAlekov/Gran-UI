#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/platform/host.hpp"

namespace
{
    void testDriverFieldCtrlEnterInput()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Resources><Command id="send" key="Mod+Enter" action="sendAll"/><Command id="history" key="Mod+Y" action="historyAll"/></Resources><Stack padding="10" spacing="8"><Input id="name"/><TextArea id="note"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        int sendAllCount = 0;
        app.onAction("sendAll", [&sendAllCount](gran::ui::Node&) { ++sendAllCount; });

        driver.click(*app.find("name"));
        driver.press(gran::platform::Key::ENTER, gran::platform::MODIFIER_CONTROL);
        TEST_ASSERT(sendAllCount == 1);
    }

    void testDriverFieldCtrlEnterTextArea()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Resources><Command id="send" key="Mod+Enter" action="sendAll"/><Command id="history" key="Mod+Y" action="historyAll"/></Resources><Stack padding="10" spacing="8"><Input id="name"/><TextArea id="note"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        int sendAllCount = 0;
        app.onAction("sendAll", [&sendAllCount](gran::ui::Node&) { ++sendAllCount; });

        driver.click(*app.find("note"));
        driver.press(gran::platform::Key::ENTER, gran::platform::MODIFIER_CONTROL);
        TEST_ASSERT(sendAllCount == 0);
    }

    void testDriverFieldMetaCharacter()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Resources><Command id="send" key="Mod+Enter" action="sendAll"/><Command id="history" key="Mod+Y" action="historyAll"/></Resources><Stack padding="10" spacing="8"><Input id="name"/><TextArea id="note"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        driver.click(*app.find("name"));
        gran::platform::Event e;
        e.kind = gran::platform::EventKind::CHARACTER;
        e.character = U'x';
        e.modifiers = gran::platform::MODIFIER_META;
        driver.window(0).pushEvent(e);
        driver.settle();
        TEST_ASSERT(app.find("name")->content.text.empty());

        driver.type("y");
        TEST_ASSERT(app.find("name")->content.text == "y");
    }

    void testDriverFieldModY()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Resources><Command id="send" key="Mod+Enter" action="sendAll"/><Command id="history" key="Mod+Y" action="historyAll"/></Resources><Stack padding="10" spacing="8"><Input id="name"/><TextArea id="note"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        int historyAllCount = 0;
        app.onAction("historyAll", [&historyAllCount](gran::ui::Node&) { ++historyAllCount; });

        driver.click(*app.find("name"));
        driver.press(gran::platform::Key::Y, gran::platform::MODIFIER_CONTROL);

        if (gran::platform::hostOs() == gran::platform::HostOs::MACOS) {
            TEST_ASSERT(historyAllCount == 1);
        } else {
            TEST_ASSERT(historyAllCount == 0);
        }
    }
}

namespace suites
{
    void registerDriverFieldTests(test::TestRunner& runner)
    {
        runner.add("driver field: Ctrl+Enter in Input", testDriverFieldCtrlEnterInput);
        runner.add("driver field: Ctrl+Enter in TextArea", testDriverFieldCtrlEnterTextArea);
        runner.add("driver field: Meta character not typed", testDriverFieldMetaCharacter);
        runner.add("driver field: Mod+Y in Input", testDriverFieldModY);
    }
}
