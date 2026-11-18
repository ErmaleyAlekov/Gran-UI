#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/ui/data_grid.hpp"

#include <string>

// Регрессионные тесты по ручной проверке Т-64 и ревью Т-66 (2.2.3)
// The regression tests from the manual check T-64 and the review T-66 (2.2.3)
namespace
{
    gran::AppOptions headless()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        return options;
    }

    void pointer(gran::AppDriver& driver, gran::platform::EventKind kind, gran::Vec2 point)
    {
        gran::platform::Event event;
        event.kind = kind;
        event.position = point;
        event.button = gran::platform::MouseButton::LEFT;
        driver.window().pushEvent(event);
        driver.settle();
    }

    // Т-64: столбцы только px, таблица шире — остаток последнему столбцу (в пределах maxWidth) / T-64: px-only columns, a wider table — the rest goes to the last column (within maxWidth)
    void testFillLastColumn()
    {
        gran::Application app(headless());
        app.loadString(R"(<Window width="400" height="200"><Resources><Data name="rows"><Item a="1" b="2"/></Data></Resources>
            <Stack><DataGrid id="g" items="rows" height="150"><GridColumn field="a" width="50"/><GridColumn field="b" width="100"/></DataGrid>
            <DataGrid id="m" items="rows" height="40"><GridColumn field="a" width="50"/><GridColumn field="b" width="100" maxWidth="200"/>
            </DataGrid></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::GridState& state = *app.find("g")->content.grid;
        TEST_ASSERT_NEAR(state.widths[0], 50.0f, 0.5f);
        TEST_ASSERT_NEAR(state.widths[0] + state.widths[1], app.find("g")->layout.frame.width - 2.0f, 0.5f);
        TEST_ASSERT_NEAR(app.find("m")->content.grid->widths[1], 200.0f, 0.5f);

        // Ячейка последнего столбца доходит до правого края таблицы (рамка текущей ячейки и фокус — во всю зону)
        // The last column cell reaches the table right edge (the current cell frame and the focus — over the whole zone)
        gran::ui::Node& cell = *gran::ui::gridHeader(*app.find("g"))->children[1];
        const gran::Rect& grid = app.find("g")->layout.frame;
        TEST_ASSERT_NEAR(cell.layout.frame.x + cell.layout.frame.width, grid.x + grid.width - 1.0f, 0.5f);
    }

    // Т-64: закреплённые строки на прокси сортируются щелчком по заголовку вместе с таблицей / T-64: pinned rows on a proxy are sorted by a header click together with the table
    void testPinnedSorted()
    {
        gran::Application app(headless());
        app.loadString(R"(<Window width="400" height="300"><Resources>
<Data name="orders"><Column name="sum" type="number"/>
<Item client="Вера" sum="10"/><Item client="Анна" sum="5"/><Item client="Борис" sum="30"/></Data>
<SortFilter name="view" source="orders"/><SortFilter name="top" source="orders" filter="@sum > 6"/></Resources>
<DataGrid id="g" items="view" pinnedTop="top" grow="1"><GridColumn field="client" width="1fr"/>
<GridColumn field="sum" width="100"/></DataGrid></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::Node& pinned = *grid.children[1];
        gran::ui::Node& header = *gran::ui::gridHeader(grid);
        TEST_ASSERT(pinned.content.items == "top" && pinned.children.size() == 2);

        driver.click(*header.children[1]);
        driver.click(*header.children[1]);
        TEST_ASSERT(grid.content.grid->descending);
        TEST_ASSERT(pinned.children[0]->children[1]->children[0]->content.text == "30");
        TEST_ASSERT(pinned.children[1]->children[1]->children[0]->content.text == "10");
    }

    // Т-66: пустая таблица шире окна — прокрутка вбок не разводит заголовок и смещение прокрутки / T-66: an empty table wider than the viewport — scrolling sideways does not split the header and the scroll offset
    void testEmptyGridScroll()
    {
        gran::Application app(headless());
        app.loadString(R"(<Window width="400" height="300"><Resources><Data name="rows"/></Resources>
            <DataGrid id="g" items="rows" grow="1"><GridColumn field="a" width="300"/><GridColumn field="b" width="300"/>
            </DataGrid></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::Node& scroll = *gran::ui::gridRows(grid)->parent;
        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_WHEEL;
        event.position = driver.centerOf(scroll);
        event.wheel = gran::Vec2{0.0f, -3.0f};
        event.modifiers = gran::platform::MODIFIER_SHIFT;
        driver.window().pushEvent(event);
        driver.settle();
        TEST_ASSERT(grid.content.grid->view.scrollX > 100.0f);
        TEST_ASSERT_NEAR(scroll.layout.scrollOffset.x, grid.content.grid->view.scrollX, 0.01f);
        TEST_ASSERT_NEAR(gran::ui::gridHeader(grid)->children[0]->layout.frame.x,
            scroll.layout.frame.x - grid.content.grid->view.scrollX, 0.5f);
    }

    // Окно видимых столбцов при любом смещении совпадает с полосой видимости по рамкам заголовка / The visible column window at any offset matches the visibility strip by the header frames
    void testShownRangeSweep()
    {
        std::string markup = R"(<Window width="400" height="200"><Resources><Data name="rows"><Item c0="x"/></Data></Resources>
<DataGrid id="g" items="rows" grow="1" frozenColumns="1">)";
        for (int index = 0; index < 12; ++index)
            markup += "<GridColumn field=\"c" + std::to_string(index) + "\" width=\"" + std::to_string(60 + index * 7) + "\"/>";
        markup += "</DataGrid></Window>";
        gran::Application app(headless());
        app.loadString(markup);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::Node& scroll = *gran::ui::gridRows(grid)->parent;
        gran::ui::Node& header = *gran::ui::gridHeader(grid);
        for (float offset = 0.0f; offset < 700.0f; offset += 37.0f)
        {
            scroll.layout.scrollOffset.x = offset;
            gran::ui::markDirty(scroll, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
            driver.settle();
            float edge = gran::ui::gridFrozenEdge(header);
            float right = scroll.layout.frame.x + scroll.layout.frame.width;
            for (std::size_t index = 1; index < header.children.size(); ++index)
            {
                const gran::Rect& frame = header.children[index]->layout.frame;
                bool visible = frame.x + frame.width > edge && frame.x < right;
                TEST_ASSERT(gran::ui::gridCellShown(header, index) == visible);
            }
        }
    }

    // Перестройка таблицы посреди перетаскивания заголовка — ссылка ввода снимается, движение и отпускание безопасны
    // Rebuilding the table in the middle of a header drag — the input reference is dropped, the move and the release are safe
    void testRebuildDuringDrag()
    {
        gran::Application app(headless());
        app.loadString(R"(<Window width="400" height="200"><Resources><Data name="rows"><Item a="1" b="2"/></Data></Resources>
            <DataGrid id="g" items="rows" grow="1"><GridColumn field="a" width="100"/><GridColumn field="b" width="100"/>
            </DataGrid></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::Vec2 from = driver.centerOf(*gran::ui::gridHeader(grid)->children[0]);
        pointer(driver, gran::platform::EventKind::MOUSE_MOVE, from);
        pointer(driver, gran::platform::EventKind::MOUSE_DOWN, from);
        app.setMarkupAttribute(grid, "frozenColumns", "1");
        driver.settle();
        pointer(driver, gran::platform::EventKind::MOUSE_MOVE, gran::Vec2{from.x + 150.0f, from.y});
        pointer(driver, gran::platform::EventKind::MOUSE_UP, gran::Vec2{from.x + 150.0f, from.y});
        TEST_ASSERT(grid.content.grid->columns[0].field == "a" && grid.content.grid->view.frozen == 1);
    }

    // Т-66: сохранённая ширина 0 или меньше предела — не уже GRID_MIN_RESIZE и minWidth / T-66: a saved width 0 or below the limit — not narrower than GRID_MIN_RESIZE and minWidth
    void testSavedWidthLimited()
    {
        std::vector<gran::ui::GridColumnSpec> specs(2);
        specs[0].column.key = "a";
        specs[1].column.key = "b";
        specs[1].column.minWidth = 60.0f;
        TEST_ASSERT(gran::ui::applyGridLayout(specs, "a:0 b:10"));
        TEST_ASSERT_NEAR(specs[0].column.width.max.value, gran::ui::GRID_MIN_RESIZE, 0.01f);
        TEST_ASSERT_NEAR(specs[0].column.width.min.value, gran::ui::GRID_MIN_RESIZE, 0.01f);
        TEST_ASSERT_NEAR(specs[1].column.width.max.value, 60.0f, 0.01f);
    }
}

namespace suites
{
    void registerGridReviewTests(test::TestRunner& runner)
    {
        runner.add("grid review: fill last column", testFillLastColumn);
        runner.add("grid review: pinned sorted", testPinnedSorted);
        runner.add("grid review: empty grid scroll", testEmptyGridScroll);
        runner.add("grid review: shown range sweep", testShownRangeSweep);
        runner.add("grid review: rebuild during drag", testRebuildDuringDrag);
        runner.add("grid review: saved width limited", testSavedWidthLimited);
    }
}
