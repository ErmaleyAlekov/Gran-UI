#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/core/gran_exception.hpp"
#include <thread>
#include <atomic>

namespace
{
    void testAppDriverNoHeadless()
    {
        gran::AppOptions options;
        options.headless = false;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        TEST_ASSERT_THROWS(gran::AppDriver driver(app), gran::GranException);
    }

    void testAppDriverStartWithoutLoad()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        gran::AppDriver driver(app);
        TEST_ASSERT_THROWS(driver.start(), gran::GranException);
    }

    void testAppDriverButtonClick()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Stack padding="10" spacing="8"><Button id="ok" onClick="save">OK</Button><Input id="name"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        int saveCount = 0;
        app.onAction("save", [&saveCount](gran::ui::Node&) { ++saveCount; });
        driver.click(*app.find("ok"));

        TEST_ASSERT(saveCount == 1);
        TEST_ASSERT(driver.nullStats(0) != nullptr);
        TEST_ASSERT(driver.nullStats(0)->frames > 0);
        TEST_ASSERT(driver.nullStats(0)->lastInstances > 0);

        std::vector<std::uint8_t> rgba;
        std::uint32_t width, height;
        TEST_ASSERT(!driver.readFrame(0, rgba, width, height));
    }

    void testAppDriverInputText()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Stack padding="10" spacing="8"><Button id="ok" onClick="save">OK</Button><Input id="name"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        driver.click(*app.find("name"));
        driver.type(u8"привет");

        TEST_ASSERT(app.find("name")->content.text == u8"привет");

        driver.press(gran::platform::Key::A, gran::platform::MODIFIER_CONTROL);
        driver.press(gran::platform::Key::C, gran::platform::MODIFIER_CONTROL);

        TEST_ASSERT(driver.window(0).clipboardText() == u8"привет");
    }

    void testAppDriverFileDialog()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Stack padding="10" spacing="8"><Button id="ok" onClick="save">OK</Button><Input id="name"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        driver.answerFileDialog({"a.txt"});
        auto files = app.openFileDialog(gran::FileDialogOptions{}, false);
        TEST_ASSERT(files.size() == 1 && files[0] == "a.txt");

        files = app.openFileDialog(gran::FileDialogOptions{}, false);
        TEST_ASSERT(files.empty());
    }

    void testAppDriverTimeout()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Stack padding="10" spacing="8"><Button id="ok" onClick="save">OK</Button><Input id="name"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        std::atomic<bool> flag = false;
        app.setTimeout(0.0, [&flag]() { flag = true; });
        driver.settle();

        TEST_ASSERT(flag);
    }

    void testAppDriverPost()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Stack padding="10" spacing="8"><Button id="ok" onClick="save">OK</Button><Input id="name"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        std::atomic<bool> flag = false;
        std::thread([&app, &flag]() {
            app.post([&flag]() { flag = true; });
        }).join();

        driver.step();

        TEST_ASSERT(flag);
    }

    void testAppDriverClose()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Stack padding="10" spacing="8"><Button id="ok" onClick="save">OK</Button><Input id="name"/></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();

        app.close();
        TEST_ASSERT(!driver.step());
    }
}

namespace suites
{
    void registerAppDriverTests(test::TestRunner& runner)
    {
        runner.add("driver: no headless", testAppDriverNoHeadless);
        runner.add("driver: start without load", testAppDriverStartWithoutLoad);
        runner.add("driver: button click", testAppDriverButtonClick);
        runner.add("driver: input text", testAppDriverInputText);
        runner.add("driver: file dialog", testAppDriverFileDialog);
        runner.add("driver: timeout", testAppDriverTimeout);
        runner.add("driver: post", testAppDriverPost);
        runner.add("driver: close", testAppDriverClose);
    }
}
