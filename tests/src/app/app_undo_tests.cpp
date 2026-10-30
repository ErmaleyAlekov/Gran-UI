#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/commands.hpp"
#include "gran/core/gran_exception.hpp"

#include <memory>
#include <string>
#include <vector>

namespace
{
    void testAppUndoInitial()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Command id="undo" key="Mod+Z" action="gran:undo"/><Command id="redo" key="Mod+Y" action="gran:redo"/><Command id="docUndo" action="gran:undo:doc"/></Resources><Stack><Button id="u" text="U" command="undo"/><Input id="field"/></Stack></Window>)");
        TEST_ASSERT(!app.commandEnabled("undo"));
        TEST_ASSERT(!app.commandEnabled("redo"));
        TEST_ASSERT(!app.find("u")->enabled);
    }

    void testAppUndoPush()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Command id="undo" key="Mod+Z" action="gran:undo"/><Command id="redo" key="Mod+Y" action="gran:redo"/><Command id="docUndo" action="gran:undo:doc"/></Resources><Stack><Button id="u" text="U" command="undo"/><Input id="field"/></Stack></Window>)");
        int value = 1;
        app.pushUndo([&]() { value = 0; }, [&]() { value = 1; });
        TEST_ASSERT(app.commandEnabled("undo"));
        TEST_ASSERT(app.find("u")->enabled);
        TEST_ASSERT(!app.commandEnabled("docUndo"));
        TEST_ASSERT(app.undo());
        TEST_ASSERT(value == 0);
        TEST_ASSERT(!app.commandEnabled("undo"));
        TEST_ASSERT(app.commandEnabled("redo"));
        TEST_ASSERT(app.redo());
        TEST_ASSERT(value == 1);
    }

    void testAppUndoDocStack()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Command id="undo" key="Mod+Z" action="gran:undo"/><Command id="redo" key="Mod+Y" action="gran:redo"/><Command id="docUndo" action="gran:undo:doc"/></Resources><Stack><Button id="u" text="U" command="undo"/><Input id="field"/></Stack></Window>)");
        int value = 1;
        app.pushUndo([&value]() { value = 0; }, []() {}, "doc");
        TEST_ASSERT(!app.commandEnabled("undo"));
        TEST_ASSERT(app.commandEnabled("docUndo"));
        // Команда gran:undo:doc отменяет шаг своего стека / The gran:undo:doc command undoes a step of its own stack
        TEST_ASSERT(app.runCommand("docUndo"));
        TEST_ASSERT(value == 0);
        TEST_ASSERT(!app.commandEnabled("docUndo"));
    }

    void testAppUndoOnUndoChanged()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Command id="undo" key="Mod+Z" action="gran:undo"/><Command id="redo" key="Mod+Y" action="gran:redo"/><Command id="docUndo" action="gran:undo:doc"/></Resources><Stack><Button id="u" text="U" command="undo"/><Input id="field"/></Stack></Window>)");
        std::vector<std::string> stackNames;
        app.onUndoChanged([&](const std::string& name, const gran::UndoStack&) {
            stackNames.push_back(name);
        });
        app.pushUndo([]() {}, []() {});
        app.pushUndo([]() {}, []() {}, "doc");
        TEST_ASSERT(stackNames.size() == 2);
        TEST_ASSERT(stackNames[0] == "");
        TEST_ASSERT(stackNames[1] == "doc");
    }

    void testAppUndoCleanState()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Command id="undo" key="Mod+Z" action="gran:undo"/><Command id="redo" key="Mod+Y" action="gran:redo"/><Command id="docUndo" action="gran:undo:doc"/></Resources><Stack><Button id="u" text="U" command="undo"/><Input id="field"/></Stack></Window>)");
        app.pushUndo([]() {}, []() {});
        TEST_ASSERT(!app.undoStack().isClean());
        gran::app::executeCommand(app, "setUndoClean", "", "");
        TEST_ASSERT(app.undoStack().isClean());
        gran::app::executeCommand(app, "undo", "", "");
        TEST_ASSERT(!app.undoStack().isClean());
        TEST_ASSERT(app.undoStack().canRedo());
        gran::app::executeCommand(app, "clearUndo", "", "");
        TEST_ASSERT(app.undoStack().count() == 0);
    }

    void testAppUndoLimit()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Command id="undo" key="Mod+Z" action="gran:undo"/><Command id="redo" key="Mod+Y" action="gran:redo"/><Command id="docUndo" action="gran:undo:doc"/></Resources><Stack><Button id="u" text="U" command="undo"/><Input id="field"/></Stack></Window>)");
        gran::app::executeCommand(app, "setUndoLimit", "", "2");
        TEST_ASSERT(app.undoStack().limit() == 2);
        TEST_ASSERT_THROWS(gran::app::executeCommand(app, "setUndoLimit", "", "x"), gran::GranException);
    }

    void testAppUndoFieldHistory()
    {
        gran::Application app;
        app.loadString(R"(<Window><Input id="field"/></Window>)");
        app.find("field")->content.history = std::make_shared<gran::ui::TextHistory>();
        gran::app::writeProperty(app, *app.find("field"), "text", "abc");
        TEST_ASSERT(app.find("field")->content.history == nullptr);
        TEST_ASSERT(app.find("field")->content.text == "abc");
    }
}

namespace suites
{
    void registerAppUndoTests(test::TestRunner& runner)
    {
        runner.add("app undo: initial", testAppUndoInitial);
        runner.add("app undo: push", testAppUndoPush);
        runner.add("app undo: doc stack", testAppUndoDocStack);
        runner.add("app undo: on undo changed", testAppUndoOnUndoChanged);
        runner.add("app undo: clean state", testAppUndoCleanState);
        runner.add("app undo: limit", testAppUndoLimit);
        runner.add("app undo: field history", testAppUndoFieldHistory);
    }
}
