#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/data/tree_list.hpp"
#include "gran/data/sort_filter_proxy.hpp"
#include "gran/ui/data_grid.hpp"
#include "gran/ui/tree_view.hpp"

#include <vector>

// Помощник gridRowsOf(app, id): узлы детей gran::ui::gridRows(*app.find(id)) с kind == GRID_ROW по порядку.
// Helper gridRowsOf(app, id): child nodes of gran::ui::gridRows(*app.find(id)) with kind == GRID_ROW in order.
std::vector<gran::ui::Node*> gridRowsOf(gran::Application& app, const std::string& id)
{
    std::vector<gran::ui::Node*> rows;
    gran::ui::Node* rowsNode = gran::ui::gridRows(*app.find(id));
    if (rowsNode == nullptr)
        return rows;
    for (const auto& child : rowsNode->children)
    {
        if (child->kind == gran::ui::ElementKind::GRID_ROW)
            rows.push_back(child.get());
    }
    return rows;
}

// Помощник filterInput(app, id, column): gran::ui::gridFilterRow(*app.find(id))->children[column]->children[0].get() (nullptr, если у ячейки нет детей).
// Helper filterInput(app, id, column): gran::ui::gridFilterRow(*app.find(id))->children[column]->children[0].get() (nullptr, if the cell has no children).
gran::ui::Node* filterInput(gran::Application& app, const std::string& id, std::size_t column)
{
    gran::ui::Node* filterRow = gran::ui::gridFilterRow(*app.find(id));
    if (filterRow == nullptr || column >= filterRow->children.size())
        return nullptr;
    gran::ui::Node* cell = filterRow->children[column].get();
    if (cell->children.empty())
        return nullptr;
    return cell->children[0].get();
}

// Тесты DataGrid
// DataGrid tests
namespace
{
    const char* const MARKUP = R"(<Window width="500" height="400"><Resources>
<Tree name="org"><Item name="Бухгалтерия" staff="6"><Item name="Расчёт" staff="2"/><Item name="Налоги" staff="3"/></Item><Item name="Склад" staff="11"/></Tree>
<TreeList name="rows" tree="org" expanded="1"/>
<Selection name="sel" model="rows"/>
<Data name="orders"><Column name="sum" type="number"/><Item client="Анна" sum="5"/><Item client="Борис" sum="30"/><Item client="Вера" sum="10"/></Data>
<SortFilter name="view" source="orders"/>
<Selection name="osel" model="view"/>
</Resources>
<Stack>
<DataGrid id="tg" items="rows" selection="sel" filterRow="true" height="160">
<GridColumn field="staff" title="Штат" width="80"/>
<GridColumn field="name" title="Отдел" tree="true" width="1fr"/>
</DataGrid>
<DataGrid id="og" items="view" selection="osel" filterRow="true" height="160">
<GridColumn field="client" title="Клиент" width="1fr"/>
<GridColumn field="sum" title="Сумма" width="100"/>
<GridColumn field="note" title="Примечание" width="80" filterable="false"/>
</DataGrid>
</Stack></Window>)";

    gran::AppOptions headless()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        return options;
    }

    // Столбец-дерево: у каждой строки "tg" (4 строки) ячейка children[1] имеет первым ребёнком узел kind TREE_EXPANDER, ячейка children[0] — нет (её первый ребёнок kind TEXT).
    // Tree column: each row "tg" (4 rows) cell children[1] has first child node kind TREE_EXPANDER, cell children[0] does not (its first child kind TEXT).
    void testTreeGridColumn()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        std::vector<gran::ui::Node*> rows = gridRowsOf(app, "tg");
        TEST_ASSERT(rows.size() == 4);
        for (const auto& row : rows)
        {
            TEST_ASSERT(row->children[1]->children[0]->kind == gran::ui::ElementKind::TREE_EXPANDER);
            TEST_ASSERT(row->children[0]->children[0]->kind == gran::ui::ElementKind::TEXT);
        }
    }

    // Стрелки в столбце-дереве: driver.click(*gridRowsOf(app,"tg")[0]->children[1]) -> app.find("tg")->content.grid->current == 1; driver.press(Key::LEFT) -> app.treeList("rows").size() == 2 и current всё ещё 1; press(Key::RIGHT) -> size() == 4. Затем driver.click по ячейке children[0] строки 0, press(Key::RIGHT) -> current == 1 и size() == 4 (не в столбце-дереве стрелка двигает столбец).
    // Arrows in tree column: driver.click(*gridRowsOf(app,"tg")[0]->children[1]) -> app.find("tg")->content.grid->current == 1; driver.press(Key::LEFT) -> app.treeList("rows").size() == 2 and current is still 1; press(Key::RIGHT) -> size() == 4. Then driver.click on cell children[0] of row 0, press(Key::RIGHT) -> current == 1 and size() == 4 (not in tree column, arrow moves the column).
    void testTreeGridArrows()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        std::vector<gran::ui::Node*> rows = gridRowsOf(app, "tg");
        driver.click(*rows[0]->children[1]);
        TEST_ASSERT(app.find("tg")->content.grid->current == 1);
        driver.press(gran::platform::Key::LEFT);
        TEST_ASSERT(app.treeList("rows").size() == 2);
        TEST_ASSERT(app.find("tg")->content.grid->current == 1);
        driver.press(gran::platform::Key::RIGHT);
        TEST_ASSERT(app.treeList("rows").size() == 4);
        driver.click(*rows[0]->children[0]);
        driver.press(gran::platform::Key::RIGHT);
        TEST_ASSERT(app.find("tg")->content.grid->current == 1);
        TEST_ASSERT(app.treeList("rows").size() == 4);
    }

    // Строка фильтров: gran::ui::gridFilterRow(*app.find("og")) != nullptr, у неё 3 ребёнка (ячейки); filterInput(app,"og",0) и (…,1) — kind INPUT; у ячейки 2 детей нет.
    // Filter row: gran::ui::gridFilterRow(*app.find("og")) != nullptr, it has 3 children (cells); filterInput(app,"og",0) and (…,1) — kind INPUT; cell 2 has no children.
    void testFilterRow()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* filterRow = gran::ui::gridFilterRow(*app.find("og"));
        TEST_ASSERT(filterRow != nullptr);
        TEST_ASSERT(filterRow->children.size() == 3);
        TEST_ASSERT(filterInput(app, "og", 0)->kind == gran::ui::ElementKind::INPUT);
        TEST_ASSERT(filterInput(app, "og", 1)->kind == gran::ui::ElementKind::INPUT);
        TEST_ASSERT(filterInput(app, "og", 2) == nullptr);
    }

    // Подстрока: driver.click(*filterInput(app,"og",0)); driver.type("ан") -> app.proxy("view").size() == 1; driver.press(Key::BACKSPACE) дважды -> size() == 3.
    // Substring: driver.click(*filterInput(app,"og",0)); driver.type("ан") -> app.proxy("view").size() == 1; driver.press(Key::BACKSPACE) twice -> size() == 3.
    void testSubstringFilter()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* filterInputNode = filterInput(app, "og", 0);
        driver.click(*filterInputNode);
        driver.type("ан");
        TEST_ASSERT(app.proxy("view").size() == 1);
        driver.press(gran::platform::Key::BACKSPACE);
        driver.press(gran::platform::Key::BACKSPACE);
        TEST_ASSERT(app.proxy("view").size() == 3);
    }

    // Ревью Т-72: таблица со строкой фильтров удалена — её условия снимаются с прокси (раньше он оставался отфильтрован)
    // Review T-72: a table with a filter row is removed — its conditions are dropped from the proxy (it used to stay filtered)
    void testFilterDroppedWithTable()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(*filterInput(app, "og", 0));
        driver.type("ан");
        TEST_ASSERT(app.proxy("view").size() == 1);
        app.remove(*app.find("og"));
        driver.settle();
        TEST_ASSERT(app.proxy("view").size() == 3);
    }

    // Ревью Т-72: документ перезагружен, у таблицы больше нет filterRow — прокси не остаётся отфильтрован прежними условиями
    // Review T-72: the document is reloaded and the table has no filterRow anymore — the proxy does not stay filtered by the former conditions
    void testFilterDroppedWithoutFilterRow()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(*filterInput(app, "og", 0));
        driver.type("ан");
        std::string plain = MARKUP;
        std::string row = "selection=\"osel\" filterRow=\"true\"";
        plain.replace(plain.find(row), row.size(), "selection=\"osel\"");
        app.loadString(plain);
        driver.settle();
        TEST_ASSERT(gran::ui::gridFilterRow(*app.find("og")) == nullptr);
        TEST_ASSERT(app.proxy("view").size() == 3);
    }

    // Вторая таблица без строки фильтров над тем же прокси удалена — фильтр первой не трогается
    // A second table without a filter row over the same proxy is removed — the first table filter is not touched
    void testOtherTableKeepsFilter()
    {
        std::string markup = MARKUP;
        markup.replace(markup.find("</Stack></Window>"), 0,
            "<DataGrid id=\"plain\" items=\"view\" height=\"80\"><GridColumn field=\"client\"/></DataGrid>");
        gran::Application app(headless());
        app.loadString(markup);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(*filterInput(app, "og", 0));
        driver.type("ан");
        app.remove(*app.find("plain"));
        driver.settle();
        TEST_ASSERT(app.proxy("view").size() == 1);
        TEST_ASSERT(filterInput(app, "og", 0)->content.text == "ан");
    }

    // Таблица перестроена при перезагрузке (сменилась её разметка): текст поля фильтра переносится, и фильтр прокси — по нему же
    // (раньше поле показывало «ан», а строки были не отфильтрованы) / The table is rebuilt on reload (its markup changed): the filter
    // input text is carried over, and the proxy filter follows it (the input used to show "ан" while the rows were unfiltered)
    void testFilterKeptOnRebuild()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(*filterInput(app, "og", 0));
        driver.type("ан");
        TEST_ASSERT(app.proxy("view").size() == 1);
        std::string changed = MARKUP;
        changed.replace(changed.find("selection=\"osel\" filterRow=\"true\" height=\"160\""),
            std::string("selection=\"osel\" filterRow=\"true\" height=\"160\"").size(),
            "selection=\"osel\" filterRow=\"true\" height=\"170\"");
        app.loadString(changed);
        driver.settle();
        std::size_t shown = app.proxy("view").size();
        std::string text = filterInput(app, "og", 0)->content.text;
        TEST_ASSERT(text == "ан");
        TEST_ASSERT(shown == 1);
        TEST_ASSERT(app.find("og")->content.grid->columns[0].filterText == "ан");
    }

    // Числа: driver.click(*filterInput(app,"og",1)); driver.type(">=10") -> app.proxy("view").size() == 2; стереть 4 раза BACKSPACE, type("abc") -> size() == 0.
    // Numbers: driver.click(*filterInput(app,"og",1)); driver.type(">=10") -> app.proxy("view").size() == 2; erase 4 times BACKSPACE, type("abc") -> size() == 0.
    void testNumberFilter()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* filterInputNode = filterInput(app, "og", 1);
        driver.click(*filterInputNode);
        driver.type(">=10");
        TEST_ASSERT(app.proxy("view").size() == 2);
        for (int i = 0; i < 4; ++i)
            driver.press(gran::platform::Key::BACKSPACE);
        driver.type("abc");
        TEST_ASSERT(app.proxy("view").size() == 0);
    }

    // Дерево: driver.click(*filterInput(app,"tg",1)); driver.type("налог") -> app.treeList("rows").size() == 2 (Бухгалтерия и Налоги).
    // Tree: driver.click(*filterInput(app,"tg",1)); driver.type("налог") -> app.treeList("rows").size() == 2 (Бухгалтерия and Налоги).
    void testTreeFilter()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* filterInputNode = filterInput(app, "tg", 1);
        driver.click(*filterInputNode);
        driver.type("налог");
        TEST_ASSERT(app.treeList("rows").size() == 2);
    }

    // Поверх фильтра разметки: отдельная разметка, как MARKUP, но <SortFilter name="view" source="orders" filter="number(@sum) &lt; 20"/> -> size() == 2; в фильтр столбца 0 type("в") -> size() == 1; BACKSPACE -> size() == 2 (фильтр разметки остался).
    // Above filter markup: separate markup, as MARKUP, but <SortFilter name="view" source="orders" filter="number(@sum) &lt; 20"/> -> size() == 2; in column 0 filter type("в") -> size() == 1; BACKSPACE -> size() == 2 (markup filter remained).
    void testAboveFilter()
    {
        const char* const FILTER_MARKUP = R"(<Window width="500" height="400"><Resources>
<Tree name="org"><Item name="Бухгалтерия" staff="6"><Item name="Расчёт" staff="2"/><Item name="Налоги" staff="3"/></Item><Item name="Склад" staff="11"/></Tree>
<TreeList name="rows" tree="org" expanded="1"/>
<Selection name="sel" model="rows"/>
<Data name="orders"><Column name="sum" type="number"/><Item client="Анна" sum="5"/><Item client="Борис" sum="30"/><Item client="Вера" sum="10"/></Data>
<SortFilter name="view" source="orders" filter="number(@sum) &lt; 20"/>
<Selection name="osel" model="view"/>
</Resources>
<Stack>
<DataGrid id="tg" items="rows" selection="sel" filterRow="true" height="160">
<GridColumn field="staff" title="Штат" width="80"/>
<GridColumn field="name" title="Отдел" tree="true" width="1fr"/>
</DataGrid>
<DataGrid id="og" items="view" selection="osel" filterRow="true" height="160">
<GridColumn field="client" title="Клиент" width="1fr"/>
<GridColumn field="sum" title="Сумма" width="100"/>
<GridColumn field="note" title="Примечание" width="80" filterable="false"/>
</DataGrid>
</Stack></Window>)";

        gran::Application app(headless());
        app.loadString(FILTER_MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        TEST_ASSERT(app.proxy("view").size() == 2);
        gran::ui::Node* filterInputNode = filterInput(app, "og", 0);
        driver.click(*filterInputNode);
        driver.type("в");
        TEST_ASSERT(app.proxy("view").size() == 1);
        driver.press(gran::platform::Key::BACKSPACE);
        TEST_ASSERT(app.proxy("view").size() == 2);
    }

    // Перестановка: в фильтр столбца 0 "og" type("в"); gran::ui::moveGridColumn(*app.find("og"), 0, 1); driver.settle() -> app.find("og")->content.grid->columns[1].filterText == "в", filterInput(app,"og",1)->content.text == "в", app.proxy("view").size() == 1.
    // Reordering: in column 0 filter of "og" type("в"); gran::ui::moveGridColumn(*app.find("og"), 0, 1); driver.settle() -> app.find("og")->content.grid->columns[1].filterText == "в", filterInput(app,"og",1)->content.text == "в", app.proxy("view").size() == 1.
    void testColumnReordering()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* filterInputNode = filterInput(app, "og", 0);
        driver.click(*filterInputNode);
        driver.type("в");
        gran::ui::moveGridColumn(*app.find("og"), 0, 1);
        driver.settle();
        TEST_ASSERT(app.find("og")->content.grid->columns[1].filterText == "в");
        TEST_ASSERT(filterInput(app, "og", 1)->content.text == "в");
        TEST_ASSERT(app.proxy("view").size() == 1);
    }
}

namespace suites
{
    void registerTreeGridTests(test::TestRunner& runner)
    {
        runner.add("tree grid: Tree Grid Column", testTreeGridColumn);
        runner.add("tree grid: Tree Grid Arrows", testTreeGridArrows);
        runner.add("tree grid: Filter Row", testFilterRow);
        runner.add("tree grid: Substring Filter", testSubstringFilter);
        runner.add("tree grid: Number Filter", testNumberFilter);
        runner.add("tree grid: Tree Filter", testTreeFilter);
        runner.add("tree grid: Above Filter", testAboveFilter);
        runner.add("tree grid: Column Reordering", testColumnReordering);
        runner.add("tree grid: filter kept on rebuild", testFilterKeptOnRebuild);
        runner.add("tree grid: filter dropped with the table", testFilterDroppedWithTable);
        runner.add("tree grid: filter dropped without filterRow", testFilterDroppedWithoutFilterRow);
        runner.add("tree grid: other table keeps the filter", testOtherTableKeepsFilter);
    }
}
