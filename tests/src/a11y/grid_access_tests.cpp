#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/ui/data_grid.hpp"
#include "gran/data/list_model.hpp"
#include "gran/ui/tree_view.hpp"
#include "gran/a11y/access_tree.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include <memory>
#include <string>
#include <vector>

namespace
{
    const char* const MARKUP = R"(<Window width="500" height="400"><Resources>
<Tree name="org"><Item name="Бухгалтерия" staff="6"><Item name="Расчёт" staff="2"/><Item name="Налоги" staff="3"/></Item><Item name="Склад" staff="11"/></Tree>
<TreeList name="rows" tree="org"/>
<Selection name="sel" model="rows" mode="single"/>
<TreeList name="trows" tree="org"/>
<Selection name="tsel" model="trows" mode="extended"/>
</Resources>
<Stack>
<DataGrid id="tg" items="rows" selection="sel" filterRow="true" height="160">
<GridColumn field="staff" title="Штат" width="80"/>
<GridColumn field="name" title="Отдел" tree="true" width="1fr"/>
</DataGrid>
<TreeView id="tv" items="trows" selection="tsel" field="name" height="120"/>
</Stack></Window>)";

    gran::AppOptions headless()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        return options;
    }

    std::vector<gran::ui::Node*> gridLines(gran::Application& app)
    {
        std::vector<gran::ui::Node*> lines;
        gran::ui::Node* rowsNode = gran::ui::gridRows(*app.find("tg"));
        if (rowsNode == nullptr)
            return lines;
        for (const auto& child : rowsNode->children)
        {
            if (child->kind == gran::ui::ElementKind::GRID_ROW)
                lines.push_back(child.get());
        }
        return lines;
    }

    std::vector<gran::ui::Node*> treeLines(gran::Application& app)
    {
        std::vector<gran::ui::Node*> lines;
        gran::ui::Node* rowsNode = gran::ui::treeViewRows(*app.find("tv"));
        if (rowsNode == nullptr)
            return lines;
        for (const auto& child : rowsNode->children)
        {
            if (child->kind == gran::ui::ElementKind::TREE_ROW)
                lines.push_back(child.get());
        }
        return lines;
    }

    void testGridAccessRoles()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* tg = app.find("tg");
        gran::ui::Node* tv = app.find("tv");
        TEST_ASSERT(gran::a11y::roleOf(*tg) == gran::a11y::Role::DATA_GRID);
        TEST_ASSERT(gran::a11y::roleOf(*gran::ui::gridHeader(*tg)) == gran::a11y::Role::HEADER);
        TEST_ASSERT(gran::a11y::roleOf(*gran::ui::gridHeader(*tg)->children[0]) == gran::a11y::Role::HEADER_ITEM);
        TEST_ASSERT(gran::a11y::roleOf(*gran::ui::gridFilterRow(*tg)) == gran::a11y::Role::GROUP);
        TEST_ASSERT(gran::a11y::roleOf(*gridLines(app)[0]) == gran::a11y::Role::DATA_ITEM);
        TEST_ASSERT(gran::a11y::roleOf(*gridLines(app)[0]->children[0]) == gran::a11y::Role::CELL);
        TEST_ASSERT(gran::a11y::roleOf(*tv) == gran::a11y::Role::TREE);
        TEST_ASSERT(gran::a11y::roleOf(*treeLines(app)[0]) == gran::a11y::Role::TREE_ITEM);
        TEST_ASSERT(gran::a11y::roleOf(*tv->children[0]) == gran::a11y::Role::GROUP);
        TEST_ASSERT(gran::a11y::describe(*tv->children[0]).decorative == true);
        gran::ui::Node* expander = gran::ui::rowExpander(*treeLines(app)[0]);
        TEST_ASSERT(gran::a11y::describe(*expander).decorative == true);
        TEST_ASSERT(gran::a11y::patternsOf(*expander) == 0);
    }

    void testGridAccessPatterns()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* tg = app.find("tg");
        gran::ui::Node* tv = app.find("tv");
        TEST_ASSERT(gran::a11y::patternsOf(*tg) == static_cast<unsigned>(gran::a11y::PATTERN_GRID | gran::a11y::PATTERN_SELECTION));
        TEST_ASSERT(gran::a11y::patternsOf(*tv) == static_cast<unsigned>(gran::a11y::PATTERN_SELECTION));
        TEST_ASSERT(gran::a11y::patternsOf(*gridLines(app)[0]) == static_cast<unsigned>(gran::a11y::PATTERN_SELECT | gran::a11y::PATTERN_EXPAND));
        TEST_ASSERT(gran::a11y::patternsOf(*gridLines(app)[0]->children[0]) == static_cast<unsigned>(gran::a11y::PATTERN_GRID_ITEM));
        TEST_ASSERT(gran::a11y::patternsOf(*gridLines(app)[0]->children[1]) == static_cast<unsigned>(gran::a11y::PATTERN_GRID_ITEM | gran::a11y::PATTERN_EXPAND));
        TEST_ASSERT(gran::a11y::patternsOf(*treeLines(app)[0]) == static_cast<unsigned>(gran::a11y::PATTERN_SELECT | gran::a11y::PATTERN_EXPAND));
    }

    void testGridAccessExpandRequest()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        TEST_ASSERT(gran::a11y::expandStateOf(*gridLines(app)[0]) == gran::a11y::ExpandState::COLLAPSED);
        TEST_ASSERT(gran::a11y::expandStateOf(*gridLines(app)[1]) == gran::a11y::ExpandState::LEAF);
        gran::a11y::postRequest(*driver.window(0).state().accessTree, gran::a11y::AccessRequest{gran::a11y::Request::EXPAND, gridLines(app)[0], std::string(), 0.0f});
        driver.settle();
        TEST_ASSERT(gridLines(app).size() == 4);
        TEST_ASSERT(gran::a11y::expandStateOf(*gridLines(app)[0]) == gran::a11y::ExpandState::EXPANDED);
        gran::a11y::postRequest(*driver.window(0).state().accessTree, gran::a11y::AccessRequest{gran::a11y::Request::COLLAPSE, gridLines(app)[0], std::string(), 0.0f});
        driver.settle();
        TEST_ASSERT(gridLines(app).size() == 2);
        gran::a11y::postRequest(*driver.window(0).state().accessTree, gran::a11y::AccessRequest{gran::a11y::Request::EXPAND, gridLines(app)[1], std::string(), 0.0f});
        driver.settle();
        TEST_ASSERT(gridLines(app).size() == 2);
    }

    void testGridAccessItemSource()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::a11y::AccessTree* tree = driver.window(0).state().accessTree;
        TEST_ASSERT(tree->items != nullptr);
        gran::ui::Node* gridList = gran::ui::gridRows(*app.find("tg"));
        TEST_ASSERT(tree->items->itemCount(*gridList) == 2);
        TEST_ASSERT(tree->items->itemIndex(*gridLines(app)[1]) == 1);
        TEST_ASSERT(tree->items->itemAt(*gridList, 1) == gridLines(app)[1]);
        TEST_ASSERT(tree->items->multipleSelection(*gridList) == false);
        TEST_ASSERT(tree->items->multipleSelection(*gran::ui::treeViewRows(*app.find("tv"))) == true);
        gran::a11y::postRequest(*tree, gran::a11y::AccessRequest{gran::a11y::Request::EXPAND, gridLines(app)[0], std::string(), 0.0f});
        driver.settle();
        TEST_ASSERT(tree->items->itemDepth(*gridLines(app)[1]) == 1);
        TEST_ASSERT(tree->items->itemDepth(*gridLines(app)[0]) == 0);
        TEST_ASSERT(tree->items->itemIndex(*app.find("tg")) == gran::a11y::NO_ITEM);
    }

    void testGridAccessSelectionRequests()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::a11y::postRequest(*driver.window(0).state().accessTree, gran::a11y::AccessRequest{gran::a11y::Request::SELECT, gridLines(app)[1], std::string(), 0.0f});
        driver.settle();
        TEST_ASSERT(gran::a11y::isSelected(*gridLines(app)[1]) == true);
        TEST_ASSERT(gran::a11y::isSelected(*gridLines(app)[0]) == false);
        TEST_ASSERT(gran::a11y::selectedItems(*app.find("tg")).size() == 1);
        gran::a11y::postRequest(*driver.window(0).state().accessTree, gran::a11y::AccessRequest{gran::a11y::Request::SELECT, treeLines(app)[0], std::string(), 0.0f});
        driver.settle();
        gran::a11y::postRequest(*driver.window(0).state().accessTree, gran::a11y::AccessRequest{gran::a11y::Request::ADD_SELECT, treeLines(app)[1], std::string(), 0.0f});
        driver.settle();
        TEST_ASSERT(gran::a11y::selectedItems(*app.find("tv")).size() == 2);
        gran::a11y::postRequest(*driver.window(0).state().accessTree, gran::a11y::AccessRequest{gran::a11y::Request::REMOVE_SELECT, treeLines(app)[0], std::string(), 0.0f});
        driver.settle();
        TEST_ASSERT(gran::a11y::selectedItems(*app.find("tv")).size() == 1);
        TEST_ASSERT(gran::a11y::isSelected(*treeLines(app)[1]) == true);
    }

    void testGridAccessFocusFollowsCurrentRow()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::a11y::postRequest(*driver.window(0).state().accessTree, gran::a11y::AccessRequest{gran::a11y::Request::SELECT, gridLines(app)[1], std::string(), 0.0f});
        driver.settle();
        gran::a11y::AccessTree* tree = driver.window(0).state().accessTree;
        TEST_ASSERT(tree->focus == gridLines(app)[1]->children[0].get());
        gran::a11y::postRequest(*tree, gran::a11y::AccessRequest{gran::a11y::Request::FOCUS, gridLines(app)[0]->children[1].get(), std::string(), 0.0f});
        driver.settle();
        TEST_ASSERT(tree->focus == gridLines(app)[0]->children[1].get());
        gran::a11y::postRequest(*tree, gran::a11y::AccessRequest{gran::a11y::Request::SELECT, treeLines(app)[1], std::string(), 0.0f});
        driver.settle();
        TEST_ASSERT(tree->focus == treeLines(app)[1]);
    }

    void testGridAccessNames()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* tg = app.find("tg");
        TEST_ASSERT(gran::a11y::accessibleName(*gridLines(app)[0]).find("Бухгалтерия") != std::string::npos);
        TEST_ASSERT(gran::a11y::accessibleName(*gridLines(app)[0]).find("6") != std::string::npos);
        TEST_ASSERT(gran::a11y::accessibleName(*gran::ui::gridHeader(*tg)->children[1]) == "Отдел");
        TEST_ASSERT(gran::a11y::accessibleName(*treeLines(app)[0]) == "Бухгалтерия");
        // Регрессия Т-73: стрелка сортировки и пустые тексты не попадают ни в имя заголовка, ни в «вид элементов»
        // Regression T-73: the sort arrow and empty texts get neither into the header name nor into the "control view"
        gran::ui::Node& header = *gran::ui::gridHeader(*tg)->children[0];
        TEST_ASSERT(gran::a11y::describe(*header.children[1]).decorative);
        gran::ui::markSortedColumn(*tg, 0, false);
        TEST_ASSERT(header.children[1]->content.text == "▲");
        TEST_ASSERT(gran::a11y::accessibleName(header) == "Штат");
        TEST_ASSERT(gran::a11y::describe(*header.children[1]).decorative);
        TEST_ASSERT(!gran::a11y::describe(*header.children[0]).decorative);
    }

    // Т-73: общий контейнер удалённых узлов для события StructureChanged и номер узла вместо адреса
    // T-73: the common container of the removed nodes for the StructureChanged event and the node serial instead of the address
    void testGridAccessRemovalContainer()
    {
        gran::ui::Node root;
        gran::ui::Node& a = gran::ui::appendChild(root, std::make_unique<gran::ui::Node>());
        gran::ui::Node& b = gran::ui::appendChild(a, std::make_unique<gran::ui::Node>());
        gran::ui::Node& c = gran::ui::appendChild(a, std::make_unique<gran::ui::Node>());
        gran::ui::Node& d = gran::ui::appendChild(root, std::make_unique<gran::ui::Node>());
        gran::a11y::AccessTree tree;
        tree.root = &root;
        gran::a11y::noteRemoval(tree, b);
        TEST_ASSERT(tree.changed == &a);
        gran::a11y::noteRemoval(tree, c);
        TEST_ASSERT(tree.changed == &a);
        gran::a11y::noteRemoval(tree, d);
        TEST_ASSERT(tree.changed == &root);
        TEST_ASSERT(b.serial != c.serial && c.serial != d.serial && root.serial != 0);
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::a11y::AccessTree* live = driver.window(0).state().accessTree;
        gran::a11y::postRequest(*live, gran::a11y::AccessRequest{gran::a11y::Request::EXPAND, gridLines(app)[0], std::string(), 0.0f});
        driver.settle();
        // Кадр прошёл: событие разослано, поле сброшено / The frame passed: the event was sent, the field was reset
        TEST_ASSERT(live->changed == nullptr);
        gran::ui::Node* list = gran::ui::gridRows(*app.find("tg"));
        app.remove(*gridLines(app)[1]);
        TEST_ASSERT(live->changed == list);
    }

    // Задача цикла: ещё одна запись / A loop task: one more record
    struct AppendRow
    {
        gran::Application* app;

        void operator()() const
        {
            app->model("few").append(gran::data::Record{{"n", "c"}});
        }
    };

    // Т-73 (пятая попытка): построенная строка — в tree.added (провайдер просит клиентов перечитать её детей), после кадра — пусто
    // T-73 (the fifth attempt): a built row is in tree.added (the provider asks the clients to reread its children), after the frame — empty
    void testGridAccessAddedRows()
    {
        gran::Application app(headless());
        app.loadString(R"(<Window width="400" height="300"><Resources><Data name="few"><Item n="a"/></Data></Resources>
<Stack><DataGrid id="small" items="few" height="150"><GridColumn field="n" width="1fr"/></DataGrid></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        gran::a11y::AccessTree* live = driver.window(0).state().accessTree;
        // Строки, построенные при загрузке, ждут первого кадра / The rows built at loading wait for the first frame
        driver.settle();
        TEST_ASSERT(live->added.empty());
        app.model("few").append(gran::data::Record{{"n", "b"}});
        gran::ui::Node* list = gran::ui::gridRows(*app.find("small"));
        TEST_ASSERT(live->added.size() == 1);
        TEST_ASSERT(live->added.front()->parent == list);
        TEST_ASSERT(live->changed == list);
        // Кадр рисует цикл, когда окно изменилось (событие, задача post) — как у программы / The loop draws a frame when the window changed (an event, a post task) — as for a program
        app.post(AppendRow{&app});
        driver.settle();
        TEST_ASSERT(live->added.empty());
        TEST_ASSERT(live->changed == nullptr);
    }

    void testGridAccessExpanderFlagsWithoutExpressions()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* expander = gran::ui::rowExpander(*treeLines(app)[0]);
        TEST_ASSERT(expander->content.branch == true);
        TEST_ASSERT(expander->content.checked == false);
        TEST_ASSERT(expander->content.option.empty());
        TEST_ASSERT(gran::ui::rowExpander(*treeLines(app)[1])->content.branch == false);
        driver.click(*expander);
        driver.settle();
        gran::ui::Node* opened = gran::ui::rowExpander(*treeLines(app)[0]);
        TEST_ASSERT(opened->content.checked == true);
        TEST_ASSERT(gran::ui::expanderHasBranch(*opened));
    }
}

namespace suites
{
    void registerGridAccessTests(test::TestRunner& runner)
    {
        runner.add("grid access: roles", testGridAccessRoles);
        runner.add("grid access: patterns", testGridAccessPatterns);
        runner.add("grid access: expand request", testGridAccessExpandRequest);
        runner.add("grid access: item source", testGridAccessItemSource);
        runner.add("grid access: selection requests", testGridAccessSelectionRequests);
        runner.add("grid access: focus follows current row", testGridAccessFocusFollowsCurrentRow);
        runner.add("grid access: names", testGridAccessNames);
        runner.add("grid access: expander flags without expressions", testGridAccessExpanderFlagsWithoutExpressions);
        runner.add("grid access: removal container", testGridAccessRemovalContainer);
        runner.add("grid access: added rows", testGridAccessAddedRows);
    }
}
