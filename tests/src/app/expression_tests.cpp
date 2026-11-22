#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/app/commands.hpp"
#include <memory>
#include <optional>
#include <stdexcept>
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

    // Преобразователь, которому тест может временно включить ошибку.
    // A converter whose failure can be enabled temporarily by the test.
    struct FailingView
    {
        const bool& fail;

        std::string operator()(const std::string& value) const
        {
            if (fail)
                throw std::runtime_error("conversion failed");
            return value;
        }
    };

    // Сбой преобразования сохраняет текст, следующий проход повторяет вычисление.
    // A conversion failure preserves the text, and the next pass retries evaluation.
    void testExpressionConverterRetries()
    {
        gran::Application app;
        bool fail = false;

        app.loadString(R"(<Window><Resources><Variable name="label" value="old"/></Resources><Text id="t">{$label ; retry}</Text></Window>)");
        app.setConverter("retry", gran::binding::Converter{FailingView{fail}, nullptr});
        app.refreshBindings();
        app.setVariable("label", "new");
        fail = true;
        app.refreshBindings();
        TEST_ASSERT(app.find("t")->content.text == "old");
        fail = false;
        app.refreshBindings();
        TEST_ASSERT(app.find("t")->content.text == "new");
    }

    // Обратное преобразование в тесте оставляет введённый текст без изменений.
    // The reverse conversion in this test passes the input text through unchanged.
    std::string unchangedSource(const std::string& value)
    {
        return value;
    }

    // После ошибки преобразования пользовательская правка всё равно записывается в источник.
    // A user edit still reaches the source after a conversion failure.
    void testExpressionRetriesWithoutDroppingWrite()
    {
        gran::Application app;
        bool fail = false;

        app.loadString(R"(<Window><Resources><Variable name="label" value="old"/></Resources>
            <Input id="field" value="{$label ; retry}"/></Window>)");
        app.setConverter("retry", gran::binding::Converter{FailingView{fail}, unchangedSource});
        app.refreshBindings();
        app.setVariable("label", "new");
        fail = true;
        app.refreshBindings();
        TEST_ASSERT(app.find("field")->content.text == "old");
        app.find("field")->content.text = "edited";
        fail = false;
        app.refreshBindings();
        TEST_ASSERT(app.variable("label") == "edited");
        TEST_ASSERT(app.find("field")->content.text == "edited");
    }

    // Пользовательский преобразователь удаляет связанные узлы во время обхода.
    // A user converter removes bound nodes during traversal.
    struct PruneView
    {
        gran::Application& app;

        std::string operator()(const std::string& value) const
        {
            app.remove(*app.find("before"));
            app.remove(*app.find("field"));
            app.remove(*app.find("remove"));
            return value;
        }
    };

    // Удаление предыдущей и текущей привязок не пропускает следующую.
    // Removing the previous and current bindings still refreshes the next binding.
    void testExpressionConverterRemovesNodes()
    {
        gran::Application app;

        app.loadString(R"(<Window><Resources><Variable name="label" value="old"/></Resources><Stack>
            <Input id="field" value="initial"/><Text id="before">{$field.text}</Text>
            <Text id="remove">{$label ; prune}</Text><Text id="after">{$label}:{$field.text}</Text></Stack></Window>)");
        app.setConverter("prune", gran::binding::Converter{PruneView{app}, nullptr});
        app.setVariable("label", "new");
        app.refreshBindings();
        TEST_ASSERT(app.find("before") == nullptr && app.find("remove") == nullptr);
        TEST_ASSERT(app.find("after")->content.text == "new:");
    }

    // Первая запись меняет порядок строк прокси, вторая сохраняет свою исходную строку.
    // The first write reorders the proxy, and the second write keeps its original row.
    void testExpressionWritesFollowReorderedItems()
    {
        gran::Application app;

        app.loadString(R"(<Window><Resources><Data name="rows"><Item score="1"/><Item score="2"/></Data>
            <SortFilter name="sorted" source="rows" sort="score:number"/></Resources>
            <Stack id="list" items="sorted"><ItemTemplate><Input value="{score}"/></ItemTemplate></Stack></Window>)");
        gran::ui::Node& list = *app.find("list");
        list.children[0]->content.text = "4";
        list.children[1]->content.text = "3";
        app.refreshBindings();
        TEST_ASSERT(app.model("rows").at(0).at("score") == "4");
        TEST_ASSERT(app.model("rows").at(1).at("score") == "3");
        TEST_ASSERT(list.children[0]->content.text == "3" && list.children[1]->content.text == "4");
    }

    // Если фильтр уничтожил копию, её отложенная запись не переходит к соседней строке.
    // If the filter removes a copy, its pending write does not go to the neighboring row.
    void testExpressionWritesDropRemovedItems()
    {
        gran::Application app;

        app.loadString(R"(<Window><Resources><Data name="rows"><Item status="open" name="one"/>
            <Item status="open" name="two"/></Data><SortFilter name="view" source="rows" filter="@status = 'open'"/>
            </Resources><Stack id="list" items="view"><ItemTemplate><Stack><Input value="{status}"/>
            <Input value="{name}"/></Stack></ItemTemplate></Stack></Window>)");
        gran::ui::Node& list = *app.find("list");
        list.children[0]->children[0]->content.text = "closed";
        list.children[0]->children[1]->content.text = "discarded";
        app.refreshBindings();
        TEST_ASSERT(list.children.size() == 1);
        TEST_ASSERT(app.model("rows").at(0).at("name") == "one");
        TEST_ASSERT(app.model("rows").at(1).at("name") == "two");
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
        runner.add("expression: converter retries", testExpressionConverterRetries);
        runner.add("expression: converter retries without dropping write", testExpressionRetriesWithoutDroppingWrite);
        runner.add("expression: converter removes nodes", testExpressionConverterRemovesNodes);
        runner.add("expression: writes follow reordered items", testExpressionWritesFollowReorderedItems);
        runner.add("expression: writes drop removed items", testExpressionWritesDropRemovedItems);
        runner.add("expression: toggle command", testExpressionToggleCommand);
        runner.add("expression: syntax error", testExpressionSyntaxError);
        runner.add("expression: double braces", testExpressionDoubleBraces);
    }
}
