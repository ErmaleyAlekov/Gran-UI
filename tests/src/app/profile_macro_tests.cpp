#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/platform/headless/headless_window.hpp"

#include <stdexcept>

namespace
{
    void testMacroExternalEffect()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Command id="one" action="one"/><Command id="two" action="two"/>
            <Command id="external" action="external"/><Command id="macro" sequence="one external two"/>
            </Resources><Box/></Window>)");
        int model = 0;
        int external = 0;
        auto& stack = app.undoStack();
        stack.setLimit(8);
        const auto record = [&](int delta) {
            model += delta;
            app.pushUndo([&, delta] { model -= delta; }, [&, delta] { model += delta; }, "document");
        };
        app.onAction("one", [&](auto&) { record(1); });
        app.onAction("two", [&](auto&) { record(2); });
        app.onAction("external", [&](auto&) { ++external; stack.clear(); stack.setLimit(1); });
        TEST_ASSERT(app.runCommand("macro"));
        TEST_ASSERT(model == 3 && external == 1 && stack.count() == 1 && stack.limit() == 8);
        TEST_ASSERT(app.undo() && model == 0 && external == 1);
        TEST_ASSERT(app.redo() && model == 3 && external == 1);
    }

    void testNestedMacros()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Command id="one"/><Command id="two"/>
            <Command id="nested" sequence="one two"/><Command id="macro" sequence="nested one nested"/>
            </Resources><Box/></Window>)");
        std::string order;
        const auto record = [&](char marker) {
            order += marker;
            app.pushUndo([&] { order.pop_back(); }, [&, marker] { order += marker; });
        };
        app.onAction("one", [&](auto&) { record('1'); });
        app.onAction("two", [&](auto&) { record('2'); });
        TEST_ASSERT(app.runCommand("macro") && order == "12112" && app.undoStack().count() == 1);
        TEST_ASSERT(app.undo() && order.empty());
        TEST_ASSERT(app.redo() && order == "12112");
        app.onAction("two", [&](auto&) { throw std::runtime_error("stop"); });
        TEST_ASSERT_THROWS(app.runCommand("macro"), std::runtime_error);
        TEST_ASSERT(order == "121121");
    }

    void testMacroCycles()
    {
        gran::Application app;
        TEST_ASSERT_THROWS(app.loadString(R"(<Window><Resources><Command id="self" sequence="self"/>
            </Resources><Box/></Window>)"), gran::MarkupException);
        TEST_ASSERT_THROWS(app.loadString(R"(<Window><Resources><Command id="left" sequence="right"/>
            <Command id="right" sequence="left"/></Resources><Box/></Window>)"), gran::MarkupException);
        app.loadString(R"(<Window><Resources><Command id="macro" sequence="inserted.command"/>
            <Palette><CommandRef ref="macro"/><Template name="tool"><Button text="Tool" customizable="command"/>
            </Template></Palette></Resources><Stack id="tools" customizable="insert"/></Window>)");
        TEST_ASSERT_THROWS(app.importProfile("cycle", R"(<Overlay version="1">
            <Insert id="inserted" parent="tools" template="tool" sequence="macro"/></Overlay>)"), gran::MarkupException);
        TEST_ASSERT(app.profiles().empty() && !app.find("inserted"));
    }

    void testMacroSaveAfterNoPaste()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Resources><Command id="paste" action="gran:paste"/><Command id="save" action="save"/>
            <Command id="macro" sequence="paste save"/></Resources><Stack>
            <Input id="editable" value="editable"/><Input id="locked" value="locked" readOnly="true"/>
            </Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        int saves = 0;
        app.onAction("save", [&](auto&) { ++saves; });
        driver.window().setClipboardText("");
        app.focus(app.find("editable"));
        TEST_ASSERT(app.runCommand("macro") && saves == 1 && app.find("editable")->content.text == "editable");
        driver.window().setClipboardText("pasted");
        app.focus(app.find("locked"));
        TEST_ASSERT(app.runCommand("macro") && saves == 2 && app.find("locked")->content.text == "locked");
        driver.window().setClipboardText("");
    }

    void testMacroCycleIntroducedByAnAction()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window><Resources><Command id="external"/><Command id="macro" sequence="external foreign"/>
            </Resources><Box/></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        int effects = 0;
        app.onAction("external", [&](auto&) {
            if (++effects == 1)
                app.openWindowString(R"(<Window><Resources><Command id="foreign" scope="global" sequence="macro"/>
                    </Resources><Box/></Window>)");
        });
        TEST_ASSERT_THROWS(app.runCommand("macro"), gran::GranException);
        TEST_ASSERT(effects == 1);
    }

    std::string doublingCommands(const std::string& prefix, const std::string& leaf, unsigned depth)
    {
        std::string xml;
        std::string previous = leaf;
        for (unsigned level = 1; level <= depth; ++level)
        {
            const auto id = prefix + std::to_string(level);
            xml += "<Command id=\"" + id + "\" sequence=\"" + previous + ' ' + previous + "\"/>";
            previous = id;
        }
        return xml;
    }

    void testMacroStepBudget()
    {
        gran::Application app;
        app.loadString("<Window><Resources><Command id=\"leaf\"/><Command id=\"disabled\" enabled=\"false\"/>"
            + doublingCommands("p", "leaf", 40) + doublingCommands("z", "missing", 40)
            + "<Command id=\"accepted\" sequence=\"p9 missing disabled z40 p9\"/>"
              "<Command id=\"over\" sequence=\"p10 leaf\"/><Command id=\"empty\" sequence=\"z40 disabled missing\"/>"
              "</Resources><Box/></Window>");
        int effects = 0;
        app.onAction("leaf", [&](auto&) { ++effects; });
        app.onAction("disabled", [&](auto&) { TEST_ASSERT(false); });
        app.pushUndo([] {}, [] {});
        auto& stack = app.undoStack();
        stack.setClean();
        TEST_ASSERT_THROWS(app.runCommand("over"), gran::GranException);
        TEST_ASSERT_THROWS(app.runCommand("p40"), gran::GranException);
        TEST_ASSERT(app.runCommand("empty") && effects == 0);
        TEST_ASSERT(stack.count() == 1 && stack.index() == 1 && stack.isClean());
        TEST_ASSERT(app.runCommand("accepted") && effects == 1024);
        TEST_ASSERT(stack.count() == 1 && stack.index() == 1 && stack.isClean());
    }

    void testMacroDynamicStepBudget()
    {
        for (unsigned count : {1023u, 1024u})
        {
            gran::AppOptions options;
            options.headless = true;
            options.backend = gran::rhi::BackendKind::NONE;
            gran::Application app(options);
            app.loadString(R"(<Window><Resources><Command id="external"/><Command id="leaf"/>
                <Command id="macro" sequence="external foreign"/></Resources><Box/></Window>)");
            gran::AppDriver driver(app);
            driver.start();
            int opened = 0, effects = 0;
            app.onAction("external", [&](auto&) {
                ++opened;
                std::string steps = "leaf";
                for (unsigned index = 1; index < count; ++index) steps += " leaf";
                app.openWindowString("<Window><Resources><Command id=\"foreign\" scope=\"global\" sequence=\""
                    + steps + "\"/></Resources><Box/></Window>");
            });
            app.onAction("leaf", [&](auto&) { ++effects; });
            if (count == 1024)
            {
                TEST_ASSERT_THROWS(app.runCommand("macro"), gran::GranException);
                TEST_ASSERT(effects == 0);
            }
            else
                TEST_ASSERT(app.runCommand("macro") && effects == 1023);
            TEST_ASSERT(opened == 1);
        }
    }
}

namespace suites
{
    void registerProfileMacroTests(test::TestRunner& runner)
    {
        runner.add("customization: macro journals reversible actions once", testMacroExternalEffect);
        runner.add("customization: nested macros preserve order and share one undo step", testNestedMacros);
        runner.add("customization: macro self and indirect cycles are rejected", testMacroCycles);
        runner.add("customization: macro still saves after an ineffective paste", testMacroSaveAfterNoPaste);
        runner.add("customization: a command added at runtime cannot reenter the active macro", testMacroCycleIntroducedByAnAction);
        runner.add("customization: compound command budget counts executable leaf occurrences", testMacroStepBudget);
        runner.add("customization: dynamically opened commands must fit the remaining budget", testMacroDynamicStepBudget);
    }
}
