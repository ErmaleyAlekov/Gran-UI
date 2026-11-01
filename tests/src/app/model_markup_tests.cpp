#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"
#include "gran/app/application.hpp"
#include "gran/binding/list_bindings.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/layout/layout.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/markup/theme.hpp"
#include "gran/style/style_sheet.hpp"
#include "gran/ui/style_pass.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    // Тексты первого Text у копий списка по порядку / The texts of the first Text of the list copies in order
    std::string rowTexts(const gran::ui::Node& list)
    {
        std::string result;

        for (const std::unique_ptr<gran::ui::Node>& row : list.children)
        {
            if (!result.empty())
                result += ",";
            result += row->children[0]->content.text;
        }
        return result;
    }

    // Таблица в <Data>, прокси с сортировкой и фильтром-выражением: смена $query пересчитывает строки / A table in <Data>, a proxy with sorting and an expression filter: changing $query recomputes the rows
    void testMarkupProxy()
    {
        gran::Application app;
        app.loadString(R"XML(<Window><Resources>
            <Data name="orders"><Column name="sum" title="Сумма" type="number"/>
              <Item client="Анна" sum="5"/><Item client="Борис" sum="30"/><Item client="Вера" sum="10"/></Data>
            <SortFilter name="view" source="orders" sort="sum:number desc" filter="contains(@client, $query)"/>
            <Variable name="query"/></Resources>
            <Stack><Stack id="list" items="view"><ItemTemplate><Stack><Text>{client}</Text></Stack></ItemTemplate></Stack>
            <Text id="count">{count(view/Item)}</Text></Stack></Window>)XML");
        TEST_ASSERT(rowTexts(*app.find("list")) == "Борис,Вера,Анна");
        TEST_ASSERT(app.model("orders").columns().size() == 1 && app.model("orders").columns()[0].type == gran::data::ColumnType::NUMBER);
        app.setVariable("query", "р");
        app.refreshBindings();
        TEST_ASSERT(rowTexts(*app.find("list")) == "Борис,Вера" && app.find("count")->content.text == "2");
        app.model("orders").set(0, "sum", "50");
        app.model("orders").set(0, "client", "Арина");
        app.refreshBindings();
        TEST_ASSERT(rowTexts(*app.find("list")) == "Арина,Борис,Вера");
    }

    // Выделение: состояние Selected у копий, $sel.count и сумма по sel/Item / The selection: the Selected state of the copies, $sel.count and a sum over sel/Item
    void testMarkupSelection()
    {
        gran::Application app;
        std::uint16_t bit = gran::style::stateBit(gran::style::State::SELECTED);
        app.loadString(R"XML(<Window><Resources>
            <Data name="orders"><Item sum="5"/><Item sum="30"/><Item sum="10"/></Data>
            <Selection name="sel" model="orders" mode="extended"/></Resources>
            <Stack><Stack id="list" items="orders" selection="sel"><ItemTemplate><Text>{@sum}{substring('*', 1, number($selected))}</Text></ItemTemplate></Stack>
            <Text id="total">{$sel.count}:{sum(sel/Item/@sum)}</Text></Stack></Window>)XML");
        app.selection("sel").activate(0, false, false);
        app.selection("sel").activate(2, false, true);
        app.refreshBindings();
        const gran::ui::Node& list = *app.find("list");
        TEST_ASSERT((list.children[1]->stateMask & bit) != 0 && (list.children[2]->stateMask & bit) != 0);
        TEST_ASSERT(app.find("total")->content.text == "3:45" && list.children[0]->content.text == "5*");
        app.model("orders").remove(1);
        app.refreshBindings();
        TEST_ASSERT(app.find("total")->content.text == "2:15" && app.selection("sel").current() == 1);
        TEST_ASSERT_THROWS(app.loadString(R"XML(<Window><Resources><Data name="a"/><Data name="b"/><Selection name="s" model="a"/></Resources><Stack items="b" selection="s"><ItemTemplate><Text/></ItemTemplate></Stack></Window>)XML"), gran::GranException);
    }

    // Дерево в разметке и TreeList: раскрытие, глубина в шаблоне, фильтр с предками / A tree in the markup and TreeList: expansion, the depth in the template, a filter with the ancestors
    void testMarkupTreeList()
    {
        gran::Application app;
        app.loadString(R"XML(<Window><Resources>
            <Tree name="org"><Item name="Б"><Item name="Б2"/><Item name="Б1"/></Item><Item name="А"/></Tree>
            <TreeList name="rows" tree="org" expanded="1" sort="name"/></Resources>
            <Stack id="list" items="rows"><ItemTemplate><Text>{$depth}{@name}</Text></ItemTemplate></Stack></Window>)XML");
        std::string all;
        for (const std::unique_ptr<gran::ui::Node>& row : app.find("list")->children)
            all += row->content.text + ",";
        TEST_ASSERT(all == "0А,0Б,1Б1,1Б2,");
        app.treeList("rows").collapse(1);
        TEST_ASSERT(app.find("list")->children.size() == 2);
        app.setFilterExpression("rows", "@name = 'Б2'");
        app.refreshBindings();
        all.clear();
        for (const std::unique_ptr<gran::ui::Node>& row : app.find("list")->children)
            all += row->content.text + ",";
        TEST_ASSERT(all == "0Б,1Б2,");
        TEST_ASSERT(app.tree("org").childCount(app.tree("org").root()) == 2);
    }

    // Источник в разметке: путь от файла разметки, первая порция и ошибка с позицией элемента / A source in the markup: a path from the markup file, the first portion and an error with the element position
    void testMarkupSourceFile()
    {
        std::filesystem::create_directories("scratchpad/models/data");
        std::ofstream("scratchpad/models/data/rows.json", std::ios::binary | std::ios::trunc)
            << "{\"rows\": [{\"n\": 1}, {\"n\": 2}, {\"n\": 3}]}";
        std::ofstream("scratchpad/models/window.xml", std::ios::binary | std::ios::trunc)
            << R"XML(<Window><Resources><Data name="rows" source="data/rows.json" select="rows" fetch="2"/></Resources><Stack/></Window>)XML";
        std::ofstream("scratchpad/models/broken.xml", std::ios::binary | std::ios::trunc)
            << R"XML(<Window><Resources><Data name="rows" source="data/none.json"/></Resources><Stack/></Window>)XML";
        gran::Application app;
        app.loadFile("scratchpad/models/window.xml");
        TEST_ASSERT(app.model("rows").size() == 2 && app.model("rows").canFetchMore());
        app.model("rows").fetchMore();
        TEST_ASSERT(app.model("rows").size() == 3 && !app.model("rows").canFetchMore());
        TEST_ASSERT_THROWS(app.loadFile("scratchpad/models/broken.xml"), gran::MarkupException);
        TEST_ASSERT_THROWS(app.loadString(R"XML(<Window><Resources><SortFilter name="v" source="nothing"/></Resources><Stack/></Window>)XML"), gran::GranException);
    }

    // Компонент получает имя модели параметром type="model" и показывает её записи / A component gets a model name by a type="model" parameter and shows its records
    void testComponentModelParam()
    {
        std::filesystem::create_directories("scratchpad/models");
        std::ofstream("scratchpad/models/lib.xml", std::ios::binary | std::ios::trunc) << R"XML(<Library namespace="urn:models">
            <Component name="Rows"><Param name="rows" type="model" required="true"/><Param name="sel" type="model"/>
            <Stack items="{$rows}" selection="{$sel}"><ItemTemplate><Text>{n}</Text></ItemTemplate></Stack></Component></Library>)XML";
        gran::Application app;
        app.loadString(R"XML(<Window xmlns:xi="http://www.w3.org/2001/XInclude" xmlns:m="urn:models"><Resources>
            <Data name="nums"><Item n="1"/><Item n="2"/></Data><Selection name="pick" model="nums"/>
            <xi:include href="scratchpad/models/lib.xml"/></Resources><m:Rows id="list" rows="nums" sel="pick"/></Window>)XML");
        TEST_ASSERT(app.find("list") != nullptr && app.find("list")->children.size() == 2);
        TEST_ASSERT(app.find("list")->content.selection == "pick");
    }

    // Клавиши списка с выделением: стрелки, Shift, Ctrl+A; у дерева — вправо раскрывает / The keys of a list with a selection: arrows, Shift, Ctrl+A; for a tree — right expands
    void testListKeys()
    {
        auto doc = gran::markup::loadMarkupString(R"XML(<Window><Resources>
            <Data name="rows"><Item n="0"/><Item n="1"/><Item n="2"/><Item n="3"/></Data>
            <Selection name="sel" model="rows"/></Resources>
            <Stack id="list" items="rows" selection="sel"><ItemTemplate><Text>{n}</Text></ItemTemplate></Stack></Window>)XML",
            gran::markup::LoadOptions{});
        gran::binding::ListBindings lists;
        lists.adoptData(doc.data);
        lists.bind(*doc.root);
        const gran::ui::Node& list = *gran::ui::findNode(*doc.root, "list");
        gran::data::SelectionModel& selection = *lists.selection("sel");
        TEST_ASSERT(lists.pressItem(list, *list.children[1], false, false));
        TEST_ASSERT(selection.current() == 1 && selection.count() == 1);
        lists.keyItem(list, gran::binding::ListKey::DOWN, false, true);
        lists.keyItem(list, gran::binding::ListKey::END, false, true);
        TEST_ASSERT(selection.current() == 3 && selection.selected() == std::vector<std::size_t>({1, 2, 3}));
        lists.keyItem(list, gran::binding::ListKey::HOME, false, false);
        TEST_ASSERT(selection.selected() == std::vector<std::size_t>({0}));
        lists.keyItem(list, gran::binding::ListKey::SELECT_ALL, true, false);
        TEST_ASSERT(selection.count() == 4);
        TEST_ASSERT(!lists.keyItem(list, gran::binding::ListKey::RIGHT, false, false));
    }
}

namespace suites
{
    void registerModelMarkupTests(test::TestRunner& runner)
    {
        runner.add("models: markup proxy", testMarkupProxy);
        runner.add("models: markup selection", testMarkupSelection);
        runner.add("models: markup tree list", testMarkupTreeList);
        runner.add("models: markup source file", testMarkupSourceFile);
        runner.add("models: list keys", testListKeys);
        runner.add("models: component model parameter", testComponentModelParam);
    }
}
