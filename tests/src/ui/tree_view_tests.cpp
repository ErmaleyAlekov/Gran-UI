#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/data/tree_list.hpp"
#include "gran/data/tree_model.hpp"
#include "gran/ui/tree_view.hpp"

#include <vector>

// Помощник rowsOf(app): узлы детей gran::ui::treeViewRows(*app.find("tv")) с kind == ElementKind::TREE_ROW по порядку (std::vector<gran::ui::Node*>).
// Helper rowsOf(app): child nodes of gran::ui::treeViewRows(*app.find("tv")) with kind == ElementKind::TREE_ROW in order (std::vector<gran::ui::Node*>).
std::vector<gran::ui::Node*> rowsOf(gran::Application& app)
{
    std::vector<gran::ui::Node*> rows;
    gran::ui::Node* rowsNode = gran::ui::treeViewRows(*app.find("tv"));
    if (rowsNode == nullptr)
        return rows;
    for (const auto& child : rowsNode->children)
    {
        if (child->kind == gran::ui::ElementKind::TREE_ROW)
            rows.push_back(child.get());
    }
    return rows;
}

// Тесты дерева <TreeView>
// TreeView tests
namespace
{
    const char* const MARKUP = R"(<Window width="400" height="300"><Resources>
<Tree name="org"><Item name="Бухгалтерия" staff="6"><Item name="Расчёт" staff="2"/><Item name="Налоги" staff="3"/></Item><Item name="Склад" staff="11"/></Tree>
<TreeList name="rows" tree="org"/>
<Selection name="sel" model="rows" mode="single"/>
</Resources>
<TreeView id="tv" items="rows" selection="sel" field="name" grow="1"/></Window>)";

    gran::AppOptions headless()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        return options;
    }

    // Построение: 2 строки (дерево свёрнуто); подписи content.text "Бухгалтерия" и "Склад"; children[0]->kind == TREE_EXPANDER; gran::ui::expanderHasBranch(*rows[0]->children[0]) == true, у строки 1 — false; rows[0]->children[0]->content.checked == false.
    // Construction: 2 rows (tree is collapsed); labels content.text "Бухгалтерия" and "Склад"; children[0]->kind == TREE_EXPANDER; gran::ui::expanderHasBranch(*rows[0]->children[0]) == true, row 1 is false; rows[0]->children[0]->content.checked == false.
    void testTreeViewConstruction()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        std::vector<gran::ui::Node*> rows = rowsOf(app);
        TEST_ASSERT(rows.size() == 2);
        TEST_ASSERT(rows[0]->children[1]->content.text == "Бухгалтерия");
        TEST_ASSERT(rows[1]->children[1]->content.text == "Склад");
        TEST_ASSERT(rows[0]->children[0]->kind == gran::ui::ElementKind::TREE_EXPANDER);
        TEST_ASSERT(rows[1]->children[0]->kind == gran::ui::ElementKind::TREE_EXPANDER);
        TEST_ASSERT(gran::ui::expanderHasBranch(*rows[0]->children[0]) == true);
        TEST_ASSERT(gran::ui::expanderHasBranch(*rows[1]->children[0]) == false);
        TEST_ASSERT(rows[0]->children[0]->content.checked == false);
    }

    // Щелчок по кнопке: driver.click(*rows[0]->children[0]) -> app.treeList("rows").size() == 4, строк 4 (rowsOf заново), у первой кнопки content.checked == true, подпись строки 1 — "Расчёт"; повторный щелчок по кнопке первой строки -> size() == 2.
    // Click on the button: driver.click(*rows[0]->children[0]) -> app.treeList("rows").size() == 4, rows 4 (rowsOf again), first button content.checked == true, label of row 1 is "Расчёт"; another click on the first row button -> size() == 2.
    void testTreeViewButtonClick()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        std::vector<gran::ui::Node*> rows = rowsOf(app);
        driver.click(*rows[0]->children[0]);
        TEST_ASSERT(app.treeList("rows").size() == 4);
        rows = rowsOf(app);
        TEST_ASSERT(rows[0]->children[0]->content.checked == true);
        TEST_ASSERT(rows[1]->children[1]->content.text == "Расчёт");
        driver.click(*rows[0]->children[0]);
        TEST_ASSERT(app.treeList("rows").size() == 2);
    }

    // Клавиатура: driver.click(*rows[0]->children[1]) (выделяет строку 0); driver.press(gran::platform::Key::RIGHT) -> size() == 4; ещё RIGHT -> app.selection("sel").current() == 1; LEFT -> current() == 0; LEFT -> size() == 2.
    // Keyboard: driver.click(*rows[0]->children[1]) (selects row 0); driver.press(gran::platform::Key::RIGHT) -> size() == 4; another RIGHT -> app.selection("sel").current() == 1; LEFT -> current() == 0; LEFT -> size() == 2.
    void testTreeViewKeyboardNavigation()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        std::vector<gran::ui::Node*> rows = rowsOf(app);
        driver.click(*rows[0]->children[1]);
        driver.press(gran::platform::Key::RIGHT);
        TEST_ASSERT(app.treeList("rows").size() == 4);
        driver.press(gran::platform::Key::RIGHT);
        TEST_ASSERT(app.selection("sel").current() == 1);
        driver.press(gran::platform::Key::LEFT);
        TEST_ASSERT(app.selection("sel").current() == 0);
        driver.press(gran::platform::Key::LEFT);
        TEST_ASSERT(app.treeList("rows").size() == 2);
    }

    // Двойной щелчок по подписи строки 0 (сначала обычный driver.click, затем MOUSE_DOWN с clicks = 2 и MOUSE_UP через driver.window().pushEvent, settle) -> size() == 4.
    // Double click on the label of row 0 (first ordinary driver.click, then MOUSE_DOWN with clicks = 2 and MOUSE_UP through driver.window().pushEvent, settle) -> size() == 4.
    void testTreeViewDoubleClick()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        std::vector<gran::ui::Node*> rows = rowsOf(app);
        driver.click(*rows[0]->children[1]);
        gran::platform::Event down;
        down.kind = gran::platform::EventKind::MOUSE_DOWN;
        down.position = driver.centerOf(*rows[0]->children[1]);
        down.button = gran::platform::MouseButton::LEFT;
        down.clicks = 2;
        driver.window().pushEvent(down);
        driver.settle();
        gran::platform::Event up;
        up.kind = gran::platform::EventKind::MOUSE_UP;
        up.position = driver.centerOf(*rows[0]->children[1]);
        up.button = gran::platform::MouseButton::LEFT;
        driver.window().pushEvent(up);
        driver.settle();
        TEST_ASSERT(app.treeList("rows").size() == 4);
    }

    // Ленивая ветка: до driver.start(): gran::data::TreeModel& tree = app.tree("org"); NodeId lazyNode = tree.append(tree.root(), Record{{"name","Ленивый"}}); tree.setLazy(lazyNode, true); tree.setFetcher(std::make_shared<CountingFetcher>()) — свой класс CountingFetcher : gran::data::TreeFetcher, fetchChildren(model, node) увеличивает счётчик calls и делает model.append(node, ...) два раза. После start: строк 3, у строки с подписью "Ленивый" expanderHasBranch == true; щелчок по её кнопке -> calls == 1, size() == 5; свернуть и раскрыть ещё раз -> calls по-прежнему 1.
    // Lazy branch: before driver.start(): gran::data::TreeModel& tree = app.tree("org"); NodeId lazyNode = tree.append(tree.root(), Record{{"name","Ленивый"}}); tree.setLazy(lazyNode, true); tree.setFetcher(std::make_shared<CountingFetcher>()) — own class CountingFetcher : gran::data::TreeFetcher, fetchChildren(model, node) increments the calls counter and does model.append(node, ...) twice. After start: rows 3, row with label "Ленивый" expanderHasBranch == true; click on its button -> calls == 1, size() == 5; collapse and expand again -> calls remains 1.
    void testTreeViewLazyBranch()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::data::TreeModel& tree = app.tree("org");
        gran::data::NodeId lazyNode = tree.append(tree.root(), gran::data::Record{{"name","Ленивый"}});
        tree.setLazy(lazyNode, true);
        class CountingFetcher : public gran::data::TreeFetcher
        {
        public:
            int calls = 0;
            void fetchChildren(gran::data::TreeModel& model, gran::data::NodeId node) override
            {
                ++calls;
                model.append(node, gran::data::Record{{"name","Дочь1"}});
                model.append(node, gran::data::Record{{"name","Дочь2"}});
            }
        };
        std::shared_ptr<CountingFetcher> fetcher = std::make_shared<CountingFetcher>();
        tree.setFetcher(fetcher);
        gran::AppDriver driver(app);
        driver.start();
        std::vector<gran::ui::Node*> rows = rowsOf(app);
        TEST_ASSERT(rows.size() == 3);
        TEST_ASSERT(gran::ui::expanderHasBranch(*rows[2]->children[0]) == true);
        driver.click(*rows[2]->children[0]);
        TEST_ASSERT(fetcher->calls == 1);
        TEST_ASSERT(app.treeList("rows").size() == 5);
        rows = rowsOf(app);
        driver.click(*rows[2]->children[0]);
        TEST_ASSERT(app.treeList("rows").size() == 3);
        rows = rowsOf(app);
        driver.click(*rows[2]->children[0]);
        TEST_ASSERT(app.treeList("rows").size() == 5);
        TEST_ASSERT(fetcher->calls == 1);
    }

    // Свой шаблон: разметка как MARKUP, но <TreeView id="tv" items="rows" indent="24"><ItemTemplate><Text>{name}: {staff}</Text></ItemTemplate></TreeView> (без field) -> rows[0]->children[1]->content.text == "Бухгалтерия: 6".
    // Own template: markup as MARKUP, but <TreeView id="tv" items="rows" indent="24"><ItemTemplate><Text>{name}: {staff}</Text></ItemTemplate></TreeView> (without field) -> rows[0]->children[1]->content.text == "Бухгалтерия: 6".
    void testTreeViewOwnTemplate()
    {
        const char* const TEMPLATE_MARKUP = R"(<Window width="400" height="300"><Resources>
<Tree name="org"><Item name="Бухгалтерия" staff="6"><Item name="Расчёт" staff="2"/><Item name="Налоги" staff="3"/></Item><Item name="Склад" staff="11"/></Tree>
<TreeList name="rows" tree="org"/>
<Selection name="sel" model="rows" mode="single"/>
</Resources>
<TreeView id="tv" items="rows" indent="24"><ItemTemplate><Text>{name}: {staff}</Text></ItemTemplate></TreeView></Window>)";

        gran::Application app(headless());
        app.loadString(TEMPLATE_MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        std::vector<gran::ui::Node*> rows = rowsOf(app);
        TEST_ASSERT(rows[0]->children[1]->content.text == "Бухгалтерия: 6");
    }

    // Отступ: в MARKUP раскрыть первую строку щелчком по кнопке; разница rows[1]->children[0]->layout.frame.x - rows[0]->children[0]->layout.frame.x равна 16 (TEST_ASSERT_NEAR с допуском 0.5).
    // Indent: in MARKUP expand the first row by clicking on the button; the difference rows[1]->children[0]->layout.frame.x - rows[0]->children[0]->layout.frame.x is 16 (TEST_ASSERT_NEAR with tolerance 0.5).
    void testTreeViewIndent()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        std::vector<gran::ui::Node*> rows = rowsOf(app);
        driver.click(*rows[0]->children[0]);
        rows = rowsOf(app);
        TEST_ASSERT_NEAR(rows[1]->children[0]->layout.frame.x - rows[0]->children[0]->layout.frame.x, 16.0f, 0.5f);
    }
}

namespace suites
{
    void registerTreeViewTests(test::TestRunner& runner)
    {
        runner.add("tree view: Construction", testTreeViewConstruction);
        runner.add("tree view: Button Click", testTreeViewButtonClick);
        runner.add("tree view: Keyboard Navigation", testTreeViewKeyboardNavigation);
        runner.add("tree view: Double Click", testTreeViewDoubleClick);
        runner.add("tree view: Lazy Branch", testTreeViewLazyBranch);
        runner.add("tree view: Own Template", testTreeViewOwnTemplate);
        runner.add("tree view: Indent", testTreeViewIndent);
    }
}
