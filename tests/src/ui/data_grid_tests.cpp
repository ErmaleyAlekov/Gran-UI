#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include "gran/ui/data_grid.hpp"
#include "gran/style/style_sheet.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/data/tree_list.hpp"
#include "support/layout_helpers.hpp"

namespace
{
    const char* MARKUP = R"(<Window width="400" height="300"><Resources>
<Data name="orders"><Column name="number" type="number"/><Column name="sum" type="number"/>
<Item number="3" client="Вера" sum="10"/><Item number="1" client="Анна" sum="5"/><Item number="2" client="Борис" sum="30"/></Data>
<SortFilter name="view" source="orders" sort="number:number"/>
<Selection name="sel" model="view" mode="extended"/></Resources>
<DataGrid id="g" items="view" selection="sel" grow="1">
<GridColumn field="number" title="№" width="50"/>
<GridColumn field="client" title="Клиент" width="1fr"/>
<GridColumn field="sum" title="Сумма" width="auto" minWidth="70"/>
</DataGrid></Window>)";

    // Номера строк в порядке детей списка (после сортировки копии переставляются — список берётся заново)
    // The row numbers in the list children order (after sorting the copies are rearranged — the list is taken anew)
    std::string rowNumbers(const gran::ui::Node& rows)
    {
        std::string numbers;
        for (const auto& row : rows.children) {
            if (row->kind == gran::ui::ElementKind::GRID_ROW) {
                if (!numbers.empty()) {
                    numbers += ",";
                }
                numbers += row->children[0]->children[0]->content.text;
            }
        }
        return numbers;
    }


    // Тест: построение таблицы
    void testDataGridBuild()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* grid = app.find("g");
        TEST_ASSERT(grid != nullptr);
        gran::ui::Node* header = gran::ui::gridHeader(*grid);
        TEST_ASSERT(header != nullptr);
        TEST_ASSERT(header->children.size() == 3);
        TEST_ASSERT(header->children[0]->children[0]->content.text == "№");
        TEST_ASSERT(header->children[1]->children[0]->content.text == "Клиент");
        TEST_ASSERT(header->children[2]->children[0]->content.text == "Сумма");
        TEST_ASSERT(header->children[0]->children[1]->content.text == "▲");
        TEST_ASSERT(header->children[0]->stateMask & gran::style::stateBit(gran::style::State::CHECKED));
        TEST_ASSERT(header->children[1]->children[1]->content.text == "");
        TEST_ASSERT(header->children[2]->children[1]->content.text == "");

        gran::ui::Node* rows = gran::ui::gridRows(*grid);
        TEST_ASSERT(rows != nullptr);
        std::vector<gran::ui::Node*> rowList;
        for (const auto& child : rows->children) {
            if (child->kind == gran::ui::ElementKind::GRID_ROW) {
                rowList.push_back(child.get());
            }
        }
        TEST_ASSERT(rowList.size() == 3);
        TEST_ASSERT(rowNumbers(*rows) == "1,2,3");
        TEST_ASSERT(rowList[0]->children.size() == 3);
        TEST_ASSERT(rowList[1]->children.size() == 3);
        TEST_ASSERT(rowList[2]->children.size() == 3);
        TEST_ASSERT(rowList[0]->children[0]->kind == gran::ui::ElementKind::GRID_CELL);
    }

    // Тест: ширины столбцов
    void testDataGridColumnWidths()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* grid = app.find("g");
        TEST_ASSERT(grid != nullptr);
        TEST_ASSERT(grid->content.grid->widths.size() == 3);
        TEST_ASSERT_NEAR(grid->content.grid->widths[0], 50.0f, 0.5f);
        TEST_ASSERT(grid->content.grid->widths[2] >= 70.0f);
        TEST_ASSERT_NEAR(grid->content.grid->widths[0] + grid->content.grid->widths[1] + grid->content.grid->widths[2], grid->layout.frame.width - 2, 1.0f);

        gran::ui::Node* header = gran::ui::gridHeader(*grid);
        TEST_ASSERT(header != nullptr);
        TEST_ASSERT(header->children[0]->layout.frame.x == header->layout.frame.x);
        TEST_ASSERT_NEAR(header->children[0]->layout.frame.width, grid->content.grid->widths[0], 0.5f);
        TEST_ASSERT(header->children[1]->layout.frame.x == header->layout.frame.x + grid->content.grid->widths[0]);
        TEST_ASSERT_NEAR(header->children[1]->layout.frame.width, grid->content.grid->widths[1], 0.5f);
        TEST_ASSERT(header->children[2]->layout.frame.x == header->layout.frame.x + grid->content.grid->widths[0] + grid->content.grid->widths[1]);
        TEST_ASSERT_NEAR(header->children[2]->layout.frame.width, grid->content.grid->widths[2], 0.5f);

        gran::ui::Node* rows = gran::ui::gridRows(*grid);
        TEST_ASSERT(rows != nullptr);
        for (const auto& child : rows->children) {
            if (child->kind == gran::ui::ElementKind::GRID_ROW) {
                TEST_ASSERT(child->children[0]->layout.frame.x == header->layout.frame.x);
                TEST_ASSERT_NEAR(child->children[0]->layout.frame.width, grid->content.grid->widths[0], 0.5f);
                TEST_ASSERT(child->children[1]->layout.frame.x == header->layout.frame.x + grid->content.grid->widths[0]);
                TEST_ASSERT_NEAR(child->children[1]->layout.frame.width, grid->content.grid->widths[1], 0.5f);
                TEST_ASSERT(child->children[2]->layout.frame.x == header->layout.frame.x + grid->content.grid->widths[0] + grid->content.grid->widths[1]);
                TEST_ASSERT_NEAR(child->children[2]->layout.frame.width, grid->content.grid->widths[2], 0.5f);
            }
        }
    }

    // Тест: текущая ячейка
    void testDataGridCurrentCell()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* grid = app.find("g");
        TEST_ASSERT(grid != nullptr);
        gran::ui::Node* rows = gran::ui::gridRows(*grid);
        TEST_ASSERT(rows != nullptr);
        std::vector<gran::ui::Node*> rowList;
        for (const auto& child : rows->children) {
            if (child->kind == gran::ui::ElementKind::GRID_ROW) {
                rowList.push_back(child.get());
            }
        }
        TEST_ASSERT(rowList.size() == 3);

        driver.click(*rowList[1]->children[1]);
        TEST_ASSERT(grid->content.grid->current == 1);
        TEST_ASSERT(rowList[1]->children[1]->stateMask & gran::style::stateBit(gran::style::State::CURRENT));
        TEST_ASSERT(!(rowList[1]->children[0]->stateMask & gran::style::stateBit(gran::style::State::CURRENT)));
        TEST_ASSERT(!(rowList[0]->children[0]->stateMask & gran::style::stateBit(gran::style::State::CURRENT)));
        TEST_ASSERT(!(rowList[2]->children[0]->stateMask & gran::style::stateBit(gran::style::State::CURRENT)));

        driver.press(gran::platform::Key::RIGHT);
        TEST_ASSERT(grid->content.grid->current == 2);
        TEST_ASSERT(rowList[1]->children[2]->stateMask & gran::style::stateBit(gran::style::State::CURRENT));
        TEST_ASSERT(!(rowList[1]->children[1]->stateMask & gran::style::stateBit(gran::style::State::CURRENT)));

        driver.press(gran::platform::Key::RIGHT);
        TEST_ASSERT(grid->content.grid->current == 2);

        driver.press(gran::platform::Key::HOME);
        TEST_ASSERT(grid->content.grid->current == 0);

        driver.press(gran::platform::Key::END);
        TEST_ASSERT(grid->content.grid->current == 2);

        driver.press(gran::platform::Key::LEFT);
        TEST_ASSERT(grid->content.grid->current == 1);

        driver.press(gran::platform::Key::DOWN);
        TEST_ASSERT(rowList[2]->children[1]->stateMask & gran::style::stateBit(gran::style::State::CURRENT));
        TEST_ASSERT(!(rowList[1]->children[1]->stateMask & gran::style::stateBit(gran::style::State::CURRENT)));
    }

    // Тест: сортировка по заголовку
    void testDataGridSortByHeader()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* grid = app.find("g");
        TEST_ASSERT(grid != nullptr);
        gran::ui::Node* header = gran::ui::gridHeader(*grid);
        TEST_ASSERT(header != nullptr);
        gran::ui::Node* rows = gran::ui::gridRows(*grid);
        TEST_ASSERT(rows != nullptr);
        std::vector<gran::ui::Node*> rowList;
        for (const auto& child : rows->children) {
            if (child->kind == gran::ui::ElementKind::GRID_ROW) {
                rowList.push_back(child.get());
            }
        }
        TEST_ASSERT(rowList.size() == 3);

        driver.click(*header->children[2]);
        TEST_ASSERT(rowNumbers(*rows) == "1,3,2");
        TEST_ASSERT(header->children[2]->children[1]->content.text == "▲");
        TEST_ASSERT(!(header->children[0]->children[1]->content.text == "▲"));
        TEST_ASSERT(!(header->children[0]->stateMask & gran::style::stateBit(gran::style::State::CHECKED)));

        driver.click(*header->children[2]);
        TEST_ASSERT(rowNumbers(*rows) == "2,3,1");
        TEST_ASSERT(header->children[2]->children[1]->content.text == "▼");

        driver.click(*header->children[1]);
        TEST_ASSERT(rowNumbers(*rows) == "1,2,3");
        TEST_ASSERT(header->children[1]->children[1]->content.text == "▲");
    }

    // Тест: без прокси
    void testDataGridNoProxy()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="400" height="300"><Resources>
<Data name="orders"><Column name="number" type="number"/><Column name="sum" type="number"/>
<Item number="3" client="Вера" sum="10"/><Item number="1" client="Анна" sum="5"/><Item number="2" client="Борис" sum="30"/></Data>
</Resources>
<DataGrid id="g" items="orders" grow="1">
<GridColumn field="number" title="№" width="50"/>
<GridColumn field="client" title="Клиент" width="1fr"/>
<GridColumn field="sum" title="Сумма" width="auto" minWidth="70"/>
</DataGrid></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* grid = app.find("g");
        TEST_ASSERT(grid != nullptr);
        gran::ui::Node* header = gran::ui::gridHeader(*grid);
        TEST_ASSERT(header != nullptr);
        gran::ui::Node* rows = gran::ui::gridRows(*grid);
        TEST_ASSERT(rows != nullptr);
        std::vector<gran::ui::Node*> rowList;
        for (const auto& child : rows->children) {
            if (child->kind == gran::ui::ElementKind::GRID_ROW) {
                rowList.push_back(child.get());
            }
        }
        TEST_ASSERT(rowList.size() == 3);

        driver.click(*header->children[2]);
        TEST_ASSERT(rowNumbers(*rows) == "3,1,2");
        TEST_ASSERT(!(header->children[2]->children[1]->content.text == "▲"));
    }
    // Строки таблицы по порядку детей списка / The table rows in the list children order
    std::vector<gran::ui::Node*> gridRowsOf(gran::ui::Node& rows)
    {
        std::vector<gran::ui::Node*> result;
        for (const auto& child : rows.children)
        {
            if (child->kind == gran::ui::ElementKind::GRID_ROW)
                result.push_back(child.get());
        }
        return result;
    }

    /*
    ** Ревью Т-63: таблица в горизонтальной прокрутке меряется при бесконечной ширине, а размещается при конечной — ширины
    ** столбцов сменяются посреди прохода. Раньше сброс кэша строк поднимал флаг перекомпоновки к уже размещённым предкам,
    ** и окно перекомпоновывалось каждый кадр в покое.
    **
    ** Review T-63: a table in a horizontal scroll is measured at an infinite width and placed at a finite one — the column
    ** widths change in the middle of a pass. Previously the row cache reset raised the relayout flag to the already placed ancestors,
    ** and the window was laid out again every frame at rest.
    */
    void testDataGridIdleAfterLayout()
    {
        auto doc = support::layoutMarkup(R"(<Window><Scroll direction="horizontal">
            <DataGrid id="g" items="none"><GridColumn field="a" title="Первый" width="1fr"/>
            <GridColumn field="b" title="Второй" width="auto"/></DataGrid></Scroll></Window>)", 600.0f, 300.0f);
        const gran::ui::Node& grid = support::nodeById(doc, "g");

        TEST_ASSERT((doc.root->dirty & gran::ui::DIRTY_LAYOUT) == 0);
        TEST_ASSERT((grid.dirty & gran::ui::DIRTY_LAYOUT) == 0);
        TEST_ASSERT_NEAR(grid.content.grid->widths[0] + grid.content.grid->widths[1], grid.layout.frame.width - 2.0f, 1.0f);
    }

    // Ревью Т-63: удалили самую длинную запись — столбец auto сужается (раньше ширина копилась навсегда) / Review T-63: the longest record was removed — the auto column narrows (previously the width accumulated forever)
    void testDataGridAutoShrinksAfterRemoval()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="600" height="300"><Resources><Data name="orders">
            <Item client="Ан"/><Item client="Очень длинное имя клиента для широкого столбца"/></Data></Resources>
            <DataGrid id="g" items="orders" grow="1"><GridColumn field="client" title="К" width="auto"/>
            <GridColumn field="x" title="X" width="1fr"/></DataGrid></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* grid = app.find("g");
        TEST_ASSERT(grid != nullptr);
        float wide = grid->content.grid->widths[0];

        // Правка — из таймера, как в программе: кадр после неё рисует цикл приложения / The edit comes from a timer, as in a program: the application loop draws the frame after it
        app.setTimeout(0.0, [&app]() { app.model("orders").remove(1); });
        driver.settle();
        TEST_ASSERT(grid->content.grid->widths[0] < wide - 10.0f);
    }

    // Ревью Т-63: таблица над плоским деревом — в первом столбце стрелки раскрывают и сворачивают, дальше — столбцы / Review T-63: a table over a flat tree — in the first column the arrows expand and collapse, further on — the columns
    void testDataGridTreeArrows()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        gran::Application app(options);
        app.loadString(R"(<Window width="400" height="300"><Resources>
            <Tree name="org"><Item name="А"><Item name="А1"/></Item><Item name="Б"/></Tree>
            <TreeList name="rows" tree="org"/><Selection name="sel" model="rows" mode="single"/></Resources>
            <DataGrid id="g" items="rows" selection="sel" grow="1"><GridColumn field="name" title="Имя"/>
            <GridColumn field="staff" title="Люди"/></DataGrid></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node* grid = app.find("g");
        TEST_ASSERT(grid != nullptr);
        gran::data::TreeList* rows = &app.treeList("rows");
        TEST_ASSERT(rows != nullptr && rows->size() == 2);

        driver.click(*gridRowsOf(*gran::ui::gridRows(*grid))[0]->children[0]);
        driver.press(gran::platform::Key::RIGHT);
        TEST_ASSERT(rows->size() == 3 && grid->content.grid->current == 0);
        driver.press(gran::platform::Key::RIGHT);
        TEST_ASSERT(grid->content.grid->current == 1);
        driver.press(gran::platform::Key::LEFT);
        TEST_ASSERT(grid->content.grid->current == 0 && rows->size() == 3);
        driver.press(gran::platform::Key::LEFT);
        TEST_ASSERT(rows->size() == 2);
    }

    // Ширина столбца — ровно одна дорожка: список через токен темы (схема его не видит) — ошибка разметки / A column width is exactly one track: a list through a theme token (the schema does not see it) is a markup error
    void testDataGridWidthOneTrack()
    {
        TEST_ASSERT_THROWS(support::layoutMarkup(R"(<Window><Resources><Theme><Token name="w" value="1fr, 2fr"/></Theme>
            </Resources><DataGrid items="none"><GridColumn field="a" width="$w"/></DataGrid></Window>)", 400, 300),
            gran::GranException);
    }
}

namespace suites
{
    void registerDataGridTests(test::TestRunner& runner)
    {
        runner.add("data grid: build", testDataGridBuild);
        runner.add("data grid: column widths", testDataGridColumnWidths);
        runner.add("data grid: current cell", testDataGridCurrentCell);
        runner.add("data grid: sort by header", testDataGridSortByHeader);
        runner.add("data grid: no proxy", testDataGridNoProxy);
        runner.add("data grid: idle after layout", testDataGridIdleAfterLayout);
        runner.add("data grid: auto shrinks after removal", testDataGridAutoShrinksAfterRemoval);
        runner.add("data grid: tree arrows", testDataGridTreeArrows);
        runner.add("data grid: width one track", testDataGridWidthOneTrack);
    }
}
