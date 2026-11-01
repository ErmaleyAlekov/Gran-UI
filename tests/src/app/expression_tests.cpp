#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/app/commands.hpp"
#include <memory>
#include <optional>
#include <string>

namespace
{
    void testExpressionCount()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Data name="files"><Item name="a" done="false"/><Item name="b" done="true"/></Data></Resources><Text id="t">Всего {count(files/Item)}</Text></Window>)");
        TEST_ASSERT(app.find("t")->content.text == "Всего 2");
        app.model("files").append(gran::data::Record{{"name","c"}});
        app.refreshBindings();
        TEST_ASSERT(app.find("t")->content.text == "Всего 3");
    }

    void testExpressionVisibility()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Variable name="flag" type="boolean" value="true"/></Resources><Box id="b" visible="{$flag}"/></Window>)");
        TEST_ASSERT(app.find("b")->visible);
        app.setVariableBoolean("flag", false);
        app.refreshBindings();
        TEST_ASSERT(!app.find("b")->visible);
    }

    void testExpressionTwoWayBinding()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Variable name="query"/></Resources><Input id="q" value="{$query}"/></Window>)");
        app.find("q")->content.text = "abc";
        app.refreshBindings();
        TEST_ASSERT(app.variable("query") == "abc");
    }

    void testExpressionList()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Data name="files"><Item name="a" done="false"/><Item name="b" done="true"/></Data></Resources><Stack id="list" items="files"><ItemTemplate><CheckBox checked="{done}"/></ItemTemplate></Stack></Window>)");
        TEST_ASSERT(!app.find("list")->children[0]->content.checked);
        TEST_ASSERT(app.find("list")->children[1]->content.checked);
        app.find("list")->children[0]->content.checked = true;
        app.refreshBindings();
        TEST_ASSERT(app.model("files").at(0).at("done") == "true");
    }

    void testExpressionNodeProperty()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack><Input id="name" value="Мир"/><Text id="t">Привет, {$name.text}</Text></Stack></Window>)");
        TEST_ASSERT(app.find("t")->content.text == "Привет, Мир");
        app.find("name")->content.text = "Гран";
        app.refreshBindings();
        TEST_ASSERT(app.find("t")->content.text == "Привет, Гран");
    }

    static std::string toView(const std::string& value)
    {
        return value + " Б";
    }

    void testExpressionConverter()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Variable name="size" type="number" value="10"/></Resources><Text id="t">{$size ; bytes}</Text></Window>)");
        app.setConverter("bytes", gran::binding::Converter{toView, nullptr});
        app.refreshBindings();
        TEST_ASSERT(app.find("t")->content.text == "10 Б");
    }

    void testExpressionToggleCommand()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Variable name="details" type="boolean" value="false"/></Resources><Box/></Window>)");
        gran::app::executeCommand(app, "toggleVariable", "details", "");
        TEST_ASSERT(app.variable("details") == "true");
    }

    void testExpressionSyntaxError()
    {
        gran::Application app;
        TEST_ASSERT_THROWS(app.loadString(R"(<Window><Text>{count(}</Text></Window>)"), gran::MarkupException);
    }

    void testExpressionDoubleBraces()
    {
        gran::Application app;
        app.loadString(R"(<Window><Text id="t">{{x}} = {1 + 1}</Text></Window>)");
        TEST_ASSERT(app.find("t")->content.text == "{x} = 2");
    }
}

namespace suites
{
    void registerExpressionTests(test::TestRunner& runner)
    {
        runner.add("expression: count", testExpressionCount);
        runner.add("expression: visibility", testExpressionVisibility);
        runner.add("expression: two-way binding", testExpressionTwoWayBinding);
        runner.add("expression: list", testExpressionList);
        runner.add("expression: node property", testExpressionNodeProperty);
        runner.add("expression: converter", testExpressionConverter);
        runner.add("expression: toggle command", testExpressionToggleCommand);
        runner.add("expression: syntax error", testExpressionSyntaxError);
        runner.add("expression: double braces", testExpressionDoubleBraces);
    }
}
