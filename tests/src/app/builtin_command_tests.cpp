#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/commands.hpp"
#include "gran/app/internal/app_state.hpp"

namespace
{
    void testBuiltinCommandWithoutPrefix()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack><Button id="button" text="Кнопка"/></Stack></Window>)");
        gran::Application::State state;
        state.owner = &app;
        TEST_ASSERT(!gran::app::executeBuiltin(state, "save", *app.find("button")));
    }

    void testBuiltinCommandMissingId()
    {
        gran::Application app;
        app.loadString(R"(<Window><Button id="button" text="Кнопка"/></Window>)");
        gran::Application::State state;
        state.owner = &app;
        TEST_ASSERT(gran::app::executeBuiltin(state, "gran:focus:missing", *app.find("button")));
    }

    void testBuiltinCommandUnknownCommand()
    {
        gran::Application app;
        app.loadString(R"(<Window><Button id="button" text="Кнопка"/></Window>)");
        gran::Application::State state;
        state.owner = &app;
        TEST_ASSERT(gran::app::executeBuiltin(state, "gran:bogus", *app.find("button")));
    }

    void testBuiltinCommandShowPopup()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack><Button text="Открыть"/><Popup id="menu"><Text>x</Text></Popup><Text id="anchor">a</Text></Stack></Window>)");
        auto* button = app.find("anchor")->parent->children.front().get();
        gran::Application::State state;
        state.owner = &app;
        TEST_ASSERT(gran::app::executeBuiltin(state, "gran:showPopup:menu", *button));
    }

    void testBuiltinCommandRemoveNode()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack><Text id="victim">Victim</Text></Stack></Window>)");
        gran::Application::State state;
        state.owner = &app;
        TEST_ASSERT(gran::app::executeBuiltin(state, "gran:remove:victim", *app.find("victim")));
        TEST_ASSERT(app.find("victim") == nullptr);
    }

    void testReloadMainWindowDoesNotAffectSecond()
    {
        gran::Application app;
        app.loadString(R"(<Window><Text id="main">Main</Text></Window>)");
        app.openWindowString(R"(<Window><Text id="second">2</Text></Window>)");
        app.loadString(R"(<Window><Text id="main2">Main2</Text></Window>)");
        TEST_ASSERT(app.find("main") == nullptr);
        TEST_ASSERT(app.find("main2") != nullptr);
        TEST_ASSERT(app.find("second") != nullptr);
    }

    void testReadAndWriteProperty()
    {
        gran::Application app;
        app.loadString(R"(<Window><Slider id="s" min="0" max="1" value="0.5"/></Window>)");
        TEST_ASSERT(gran::app::readProperty(*app.find("s"), "value") == "0.5");
        gran::app::writeProperty(app, *app.find("s"), "value", "0.25");
        TEST_ASSERT(gran::app::readProperty(*app.find("s"), "value") == "0.25");
    }
}

namespace suites
{
    void registerBuiltinCommandTests(test::TestRunner& runner)
    {
        runner.add("builtin command: without prefix", testBuiltinCommandWithoutPrefix);
        runner.add("builtin command: missing id", testBuiltinCommandMissingId);
        runner.add("builtin command: unknown command", testBuiltinCommandUnknownCommand);
        runner.add("builtin command: show popup", testBuiltinCommandShowPopup);
        runner.add("builtin command: remove node", testBuiltinCommandRemoveNode);
        runner.add("reload: main window does not affect second", testReloadMainWindowDoesNotAffectSecond);
        runner.add("read and write property", testReadAndWriteProperty);
    }
}
