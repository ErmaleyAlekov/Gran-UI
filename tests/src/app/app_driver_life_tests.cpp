#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/core/gran_exception.hpp"
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace
{
    void testAppDriverLifeAfterApp()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        std::unique_ptr<gran::Application> app = std::make_unique<gran::Application>(options);
        app->loadString(R"(<Window width="300" height="200"><Button id="ok" onClick="save">OK</Button></Window>)");
        gran::AppDriver driver(*app);
        driver.start();
        app.reset();

        TEST_ASSERT_THROWS(driver.step(), gran::GranException);
        TEST_ASSERT_THROWS(driver.settle(), gran::GranException);
        TEST_ASSERT_THROWS(driver.windowCount(), gran::GranException);
    }

    void testAppDriverBeforeApp()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        int saveCount = 0;
        app.loadString(R"(<Window width="300" height="200"><Button id="ok" onClick="save">OK</Button></Window>)");
        app.onAction("save", [&saveCount](gran::ui::Node&) { ++saveCount; });

        {
            gran::AppDriver driver(app);
            driver.start();
            driver.click(*app.find("ok"));
        }

        gran::AppDriver second(app);
        second.start();
        second.click(*app.find("ok"));

        TEST_ASSERT(saveCount == 2);
    }

    void testAppDriverDoubleStart()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="300" height="200"><Button id="ok" onClick="save">OK</Button></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        driver.start();

        std::atomic<bool> flag = false;
        std::thread([&app, &flag]() {
            app.post([&flag]() { flag = true; });
        }).join();

        // Повторный start не снял получателя пробуждений: post разбудил окно, ожидание не длится 2 с
        // A repeated start did not remove the wake target: post woke the window, the wait does not last 2 s
        std::chrono::steady_clock::time_point before = std::chrono::steady_clock::now();
        driver.window(0).waitEvents(2.0);
        TEST_ASSERT(std::chrono::steady_clock::now() - before < std::chrono::seconds(1));
        driver.settle();

        TEST_ASSERT(flag);
    }
}

namespace suites
{
    void registerAppDriverLifeTests(test::TestRunner& runner)
    {
        runner.add("driver life: after app", testAppDriverLifeAfterApp);
        runner.add("driver life: before app", testAppDriverBeforeApp);
        runner.add("driver life: double start", testAppDriverDoubleStart);
    }
}
