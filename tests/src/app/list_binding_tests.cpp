#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include <string_view>

namespace
{
    void testListBinding()
    {
        gran::Application app;
        app.loadString(R"(<Window><Resources><Style name="row" padding="4"/><Data name="people"><Item name="Ann" age="30"/><Item name="Bob" age="41"/></Data></Resources><Stack id="list" items="people"><Text id="head">Head</Text><ItemTemplate><Stack style="row"><Text>{name}</Text><Text tooltip="{age}">Age {age}</Text></Stack></ItemTemplate></Stack></Window>)");

        auto* list = app.find("list");
        TEST_ASSERT(list != nullptr);
        TEST_ASSERT(list->children.size() == 3);

        auto* firstText = list->children[1]->children[0].get();
        TEST_ASSERT(firstText->content.text == "Ann");

        auto* secondText = list->children[1]->children[1].get();
        TEST_ASSERT(secondText->content.text == "Age 30");
        TEST_ASSERT(secondText->content.tooltip == "30");

        app.model("people").append({{"name", "Cid"}, {"age", "7"}});
        TEST_ASSERT(list->children.size() == 4);

        auto* lastText = list->children[3]->children[0].get();
        TEST_ASSERT(lastText->content.text == "Cid");

        app.model("people").set(0, "name", "Anna");
        TEST_ASSERT(firstText->content.text == "Anna");

        app.model("people").remove(1);
        TEST_ASSERT(list->children.size() == 3);

        auto* updatedText = list->children[2]->children[0].get();
        TEST_ASSERT(updatedText->content.text == "Cid");

        auto* itemNode = list->children[1]->children[0].get();
        auto itemRef = app.itemOf(*itemNode);
        TEST_ASSERT(itemRef.model == &app.model("people"));
        TEST_ASSERT(itemRef.index == 0);

        auto* headNode = list->children[0].get();
        auto headRef = app.itemOf(*headNode);
        TEST_ASSERT(headRef.model == nullptr);

        app.model("people").clear();
        TEST_ASSERT(list->children.size() == 1);

        app.appendMarkup(*list, R"(<Text id="extra" style="row">X</Text>)");
        auto* extraNode = app.find("extra");
        TEST_ASSERT(extraNode != nullptr);

        TEST_ASSERT_THROWS(app.appendMarkup(*list, "<Divv/>"), gran::MarkupException);
        TEST_ASSERT_THROWS(app.appendMarkup(*extraNode, "<Text>Inner</Text>"), gran::GranException);
        TEST_ASSERT_THROWS(app.loadString("<Window><Stack items=\"people\"/></Window>"), gran::MarkupException);
    }
}

namespace suites
{
    void registerListBindingTests(test::TestRunner& runner)
    {
        runner.add("list binding: basic functionality", testListBinding);
    }
}
