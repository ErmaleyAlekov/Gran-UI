#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/markup/live_document.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include "gran/settings/settings.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/ui/data_grid.hpp"

#include <filesystem>
#include <string>

namespace
{
    // Три пиксельных столбца чуть шире таблицы (350 из 338): остаток последнему столбцу не достаётся (fillGridRest) и ширины
    // в тестах — ровно из разметки / Three pixel columns slightly wider than the table (350 of 338): the last column gets no rest (fillGridRest)
    // and the widths in the tests are exactly from the markup
    const char* BASIC = R"(<Window width="340" height="300" layout="main"><Resources>
<Data name="orders"><Column name="number" type="number"/><Column name="sum" type="number"/>
<Item number="3" client="Вера" sum="10"/><Item number="1" client="Анна" sum="5"/><Item number="2" client="Борис" sum="30"/></Data>
<SortFilter name="view" source="orders" sort="number:number"/>
<Selection name="sel" model="view" mode="extended"/></Resources>
<DataGrid id="g" items="view" selection="sel" grow="1" LAYOUT>
<GridColumn field="number" title="№" width="50"/>
<GridColumn field="client" title="Клиент" width="150"/>
<GridColumn field="sum" title="Сумма" width="150"/>
</DataGrid></Window>)";

    // Четыре столбца шире таблицы (500 из 398), первый закреплён, итоги закреплены снизу / Four columns wider than the table (500 of 398), the first one is frozen, the totals are pinned at the bottom
    const char* FROZEN = R"(<Window width="400" height="300"><Resources>
<Data name="orders"><Column name="number" type="number"/><Column name="sum" type="number"/>
<Item number="3" client="Вера" city="Тула" sum="10"/><Item number="1" client="Анна" city="Омск" sum="5"/>
<Item number="2" client="Борис" city="Псков" sum="30"/></Data>
<Data name="totals"><Item client="Итого" sum="45"/></Data>
<Selection name="sel" model="orders" mode="single"/></Resources>
<DataGrid id="g" items="orders" selection="sel" grow="1" frozenColumns="1" pinnedBottom="totals">
<GridColumn field="number" title="№" width="50"/>
<GridColumn field="client" title="Клиент" width="150"/>
<GridColumn field="city" title="Город" width="150"/>
<GridColumn field="sum" title="Сумма" width="150"/>
</DataGrid></Window>)";

    // Разметка BASIC с атрибутами таблицы вместо метки LAYOUT / The BASIC markup with the table attributes instead of the LAYOUT mark
    std::string basicWith(const std::string& attributes)
    {
        std::string markup = BASIC;
        markup.replace(markup.find("LAYOUT"), 6, attributes);
        return markup;
    }

    // Безоконное приложение без графики / A windowless application without graphics
    gran::AppOptions headless()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        return options;
    }

    // Событие мыши в точке окна; clicks — номер щелчка (2 — второй щелчок двойного) / A mouse event at a window point; clicks is the click number (2 — the second click of a double one)
    void pointer(gran::AppDriver& driver, gran::platform::EventKind kind, gran::Vec2 point, std::uint8_t clicks = 1,
        std::uint8_t modifiers = 0)
    {
        gran::platform::Event event;
        event.kind = kind;
        event.position = point;
        event.button = gran::platform::MouseButton::LEFT;
        event.clicks = clicks;
        event.modifiers = modifiers;
        driver.window().pushEvent(event);
        driver.settle();
    }

    // Протащить мышь от from к to с нажатой левой кнопкой / Drag the mouse from from to to with the left button held
    void drag(gran::AppDriver& driver, gran::Vec2 from, gran::Vec2 to)
    {
        pointer(driver, gran::platform::EventKind::MOUSE_MOVE, from);
        pointer(driver, gran::platform::EventKind::MOUSE_DOWN, from);
        pointer(driver, gran::platform::EventKind::MOUSE_MOVE, gran::Vec2{(from.x + to.x) * 0.5f, to.y});
        pointer(driver, gran::platform::EventKind::MOUSE_MOVE, to);
        pointer(driver, gran::platform::EventKind::MOUSE_UP, to);
    }

    // Колесо мыши над узлом (Shift — вбок) / The mouse wheel over a node (Shift — sideways)
    void wheel(gran::AppDriver& driver, const gran::ui::Node& node, float notches, std::uint8_t modifiers)
    {
        gran::platform::Event event;
        event.kind = gran::platform::EventKind::MOUSE_WHEEL;
        event.position = driver.centerOf(node);
        event.wheel = gran::Vec2{0.0f, notches};
        event.modifiers = modifiers;
        driver.window().pushEvent(event);
        driver.settle();
    }

    // Точка у правого края узла (внутри полосы ручки) / A point at the node right edge (inside the grip strip)
    gran::Vec2 rightEdge(const gran::ui::Node& node)
    {
        const gran::Rect& frame = node.layout.frame;
        return gran::Vec2{frame.x + frame.width - 2.0f, frame.y + frame.height * 0.5f};
    }

    // Копии строк основного списка / The main list row copies
    std::vector<gran::ui::Node*> rowsOf(gran::ui::Node& grid)
    {
        std::vector<gran::ui::Node*> rows;
        for (const auto& child : gran::ui::gridRows(grid)->children)
        {
            if (child->kind == gran::ui::ElementKind::GRID_ROW)
                rows.push_back(child.get());
        }
        return rows;
    }

    // Текст ячейки поля (Text — первый ребёнок ячейки) / The field cell text (the Text is the first cell child)
    const std::string& cellText(const gran::ui::Node& row, std::size_t column)
    {
        return row.children[column]->children[0]->content.text;
    }

    // Край заголовка: ширина растёт на сдвиг мыши, щелчка-сортировки нет / A header edge: the width grows by the mouse shift, there is no sorting click
    void testResizeByEdge()
    {
        gran::Application app(headless());
        app.loadString(basicWith(""));
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::GridState& state = *grid.content.grid;
        gran::ui::Node& header = *gran::ui::gridHeader(grid);
        gran::Vec2 edge = rightEdge(*header.children[0]);

        drag(driver, edge, gran::Vec2{edge.x + 40.0f, edge.y});
        TEST_ASSERT_NEAR(state.widths[0], 90.0f, 0.5f);
        TEST_ASSERT(state.columns[0].resized && state.view.customized);
        TEST_ASSERT(state.sorted == 0 && !state.descending);
        TEST_ASSERT_NEAR(rowsOf(grid)[0]->children[0]->layout.frame.width, 90.0f, 0.5f);
        TEST_ASSERT_NEAR(header.children[1]->layout.frame.x, header.children[0]->layout.frame.x + 90.0f, 0.5f);

        // Сужение не ниже GRID_MIN_RESIZE / Narrowing is not below GRID_MIN_RESIZE
        edge = rightEdge(*header.children[0]);
        drag(driver, edge, gran::Vec2{edge.x - 200.0f, edge.y});
        TEST_ASSERT_NEAR(state.widths[0], gran::ui::GRID_MIN_RESIZE, 0.5f);
    }

    // Двойной щелчок по краю — ширина по содержимому (auto) / A double click on the edge — the width by the content (auto)
    void testEdgeDoubleClick()
    {
        gran::Application app(headless());
        app.loadString(basicWith(""));
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::GridState& state = *grid.content.grid;
        gran::Vec2 edge = rightEdge(*gran::ui::gridHeader(grid)->children[1]);

        pointer(driver, gran::platform::EventKind::MOUSE_MOVE, edge);
        pointer(driver, gran::platform::EventKind::MOUSE_DOWN, edge);
        pointer(driver, gran::platform::EventKind::MOUSE_UP, edge);
        pointer(driver, gran::platform::EventKind::MOUSE_DOWN, edge, 2);
        pointer(driver, gran::platform::EventKind::MOUSE_UP, edge, 2);
        TEST_ASSERT(state.columns[1].width.max.kind == gran::style::TrackBoundKind::AUTO);
        TEST_ASSERT(state.columns[1].resized);
        TEST_ASSERT(state.widths[1] < 140.0f && state.widths[1] > 20.0f);
        TEST_ASSERT(state.sorted == 0);
    }

    // Перетаскивание заголовка переставляет столбец везде; сортировка идёт за столбцом / Dragging a header moves the column everywhere; the sort follows the column
    void testReorderByDrag()
    {
        gran::Application app(headless());
        app.loadString(basicWith(""));
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::GridState& state = *grid.content.grid;
        gran::ui::Node& header = *gran::ui::gridHeader(grid);
        const gran::Rect& target = header.children[2]->layout.frame;

        drag(driver, driver.centerOf(*header.children[0]),
            gran::Vec2{target.x + target.width * 0.75f, target.y + target.height * 0.5f});
        TEST_ASSERT(header.children[2]->children[0]->content.text == "№");
        TEST_ASSERT(header.children[0]->children[0]->content.text == "Клиент");
        TEST_ASSERT(state.columns[2].field == "number" && state.columns[0].field == "client");
        TEST_ASSERT(state.sorted == 2 && !state.descending);
        TEST_ASSERT(cellText(*rowsOf(grid)[0], 2) == "1" && cellText(*rowsOf(grid)[0], 0) == "Анна");
        TEST_ASSERT(gran::ui::gridRows(grid)->itemTemplate->children[2]->children[0]->content.text == "{number}");
        TEST_ASSERT(header.children[2]->layout.frame.x > header.children[1]->layout.frame.x);
        TEST_ASSERT_NEAR(state.widths[2], 50.0f, 0.5f);
    }

    // Клавиши на заголовке в фокусе: Ctrl+→ переставляет, Shift+→ расширяет / The keys on a focused header: Ctrl+→ moves, Shift+→ widens
    void testHeaderKeys()
    {
        gran::Application app(headless());
        app.loadString(basicWith(""));
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::GridState& state = *grid.content.grid;
        gran::ui::Node& header = *gran::ui::gridHeader(grid);

        driver.click(*header.children[1]);
        driver.press(gran::platform::Key::RIGHT, gran::platform::MODIFIER_CONTROL);
        TEST_ASSERT(header.children[2]->children[0]->content.text == "Клиент");
        driver.press(gran::platform::Key::RIGHT, gran::platform::MODIFIER_SHIFT);
        TEST_ASSERT_NEAR(state.widths[2], 160.0f, 0.5f);
        driver.press(gran::platform::Key::LEFT, gran::platform::MODIFIER_SHIFT);
        driver.press(gran::platform::Key::LEFT, gran::platform::MODIFIER_SHIFT);
        TEST_ASSERT_NEAR(state.widths[2], 140.0f, 0.5f);
    }

    // Запреты разметки: resizable="false" у столбца, reorderable="false" у таблицы / The markup bans: resizable="false" on a column, reorderable="false" on the table
    void testPermissions()
    {
        gran::Application app(headless());
        std::string markup = basicWith("reorderable=\"false\"");
        markup.replace(markup.find("width=\"50\""), 10, "width=\"50\" resizable=\"false\"");
        app.loadString(markup);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::Node& header = *gran::ui::gridHeader(grid);
        gran::Vec2 edge = rightEdge(*header.children[0]);
        const gran::Rect& target = header.children[2]->layout.frame;

        drag(driver, edge, gran::Vec2{edge.x + 40.0f, edge.y});
        TEST_ASSERT_NEAR(grid.content.grid->widths[0], 50.0f, 0.5f);
        drag(driver, driver.centerOf(*header.children[1]),
            gran::Vec2{target.x + target.width * 0.75f, target.y + target.height * 0.5f});
        TEST_ASSERT(header.children[1]->children[0]->content.text == "Клиент");
        TEST_ASSERT(!grid.content.grid->view.customized);
    }

    // Раскладка из разметки: порядок, ширина, неизвестный ключ пропущен; после перестановки пишется в живую разметку
    // A layout from the markup: the order, the width, the unknown key is skipped; after reordering it is written into the live markup
    void testLayoutAttribute()
    {
        gran::Application app(headless());
        app.loadString(basicWith("columnLayout=\"sum zz client:80\""));
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::GridState& state = *grid.content.grid;

        TEST_ASSERT(state.columns[0].key == "sum" && state.columns[1].key == "client" && state.columns[2].key == "number");
        TEST_ASSERT_NEAR(state.widths[1], 80.0f, 0.5f);
        gran::ui::moveGridColumn(grid, 2, 0);
        gran::markup::writeLiveAttributes(grid);
        TEST_ASSERT(app.markupAttribute(grid, "columnLayout").value_or("") == "number sum client:80");
    }

    // Раскладка переживает перезапуск: закрытие окна пишет её в настройки, новый запуск строит столбцы по ней
    // The layout survives a restart: closing the window writes it into the settings, a new start builds the columns by it
    void testLayoutSettings()
    {
        std::filesystem::path folder = std::filesystem::temp_directory_path() / "gran_grid_layout";
        std::error_code error;
        std::filesystem::remove_all(folder, error);
        gran::AppOptions options = headless();
        options.settingsFile = (folder / "settings.xml").u8string();
        {
            gran::Application app(options);
            app.loadString(basicWith(""));
            gran::AppDriver driver(app);
            driver.start();
            gran::ui::Node& grid = *app.find("g");
            gran::ui::moveGridColumn(grid, 0, 2);
            gran::ui::setGridColumnWidth(grid, 2, gran::style::parseTrack("70"));
            driver.window().close();
            driver.settle();
            TEST_ASSERT(app.settings().save());
        }
        gran::Application app(options);
        app.loadString(basicWith(""));
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::GridState& state = *app.find("g")->content.grid;
        TEST_ASSERT(state.columns[0].key == "client" && state.columns[2].key == "number");
        TEST_ASSERT_NEAR(state.widths[2], 70.0f, 0.5f);
    }

    // Испорченный сохранённый атрибут не мешает запуску: пропускается, разметка прежняя / A damaged saved attribute does not prevent the start: it is skipped, the markup stays
    void testDamagedSettingsSkipped()
    {
        std::filesystem::path folder = std::filesystem::temp_directory_path() / "gran_grid_damaged";
        std::error_code error;
        std::filesystem::remove_all(folder, error);
        gran::AppOptions options = headless();
        options.settingsFile = (folder / "settings.xml").u8string();
        {
            gran::settings::Settings settings;
            settings.load(options.settingsFile);
            settings.setAttribute({"main", "split", "position"}, "abc");
            settings.setAttribute({"main", "g", "columnLayout"}, "client:-1 :::: sum:1e999");
            TEST_ASSERT(settings.save());
        }
        gran::Application app(options);
        std::string markup = basicWith("");
        markup.replace(markup.find("<DataGrid"), 0,
            "<Stack grow=\"1\"><Splitter id=\"split\" persist=\"position\" position=\"120\"><Box/><Box/></Splitter>");
        markup.replace(markup.find("</DataGrid>"), 11, "</DataGrid></Stack>");
        app.loadString(markup);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::GridState& state = *app.find("g")->content.grid;
        TEST_ASSERT(state.columns[0].key == "client" && state.columns[1].key == "sum");
        TEST_ASSERT(!state.columns[0].resized && !state.columns[1].resized);
        TEST_ASSERT(app.markupAttribute(*app.find("split"), "position").value_or("") == "120");
    }
}

namespace
{
    // Закреплённый столбец стоит на месте, остальные и заголовок сдвигаются прокруткой вместе / The frozen column stays in place, the others and the header move together with the scroll
    void testFrozenScroll()
    {
        gran::Application app(headless());
        app.loadString(FROZEN);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::GridState& state = *grid.content.grid;
        gran::ui::Node& header = *gran::ui::gridHeader(grid);
        gran::ui::Node& row = *rowsOf(grid)[0];
        float frozenX = header.children[0]->layout.frame.x;
        float clientX = header.children[1]->layout.frame.x;

        wheel(driver, *row.children[2], -1.0f, gran::platform::MODIFIER_SHIFT);
        TEST_ASSERT_NEAR(state.view.scrollX, gran::platform::WHEEL_NOTCH_PIXELS, 0.5f);
        TEST_ASSERT_NEAR(header.children[0]->layout.frame.x, frozenX, 0.5f);
        TEST_ASSERT_NEAR(row.children[0]->layout.frame.x, frozenX, 0.5f);
        TEST_ASSERT_NEAR(header.children[1]->layout.frame.x, clientX - state.view.scrollX, 0.5f);
        TEST_ASSERT_NEAR(row.children[1]->layout.frame.x, header.children[1]->layout.frame.x, 0.5f);
        TEST_ASSERT_NEAR(gran::ui::gridFrozenEdge(row), frozenX + 50.0f, 0.5f);

        // Смещение не дальше конца столбцов; под закреплённым мышь ловит закреплённую ячейку
        // The offset is not past the column end; under the frozen one the mouse catches the frozen cell
        wheel(driver, *row.children[2], -10.0f, gran::platform::MODIFIER_SHIFT);
        TEST_ASSERT_NEAR(state.view.scrollX, 500.0f - (grid.layout.frame.width - 2.0f), 1.0f);
        driver.click(*row.children[3]);
        TEST_ASSERT(state.current == 3);
        driver.click(*row.children[0]);
        TEST_ASSERT(state.current == 0);
    }

    // Закреплённые снизу строки: под прокруткой, ячейки по тем же столбцам и сдвигу / The rows pinned at the bottom: below the scroll, the cells by the same columns and shift
    void testPinnedRows()
    {
        gran::Application app(headless());
        app.loadString(FROZEN);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::Node& header = *gran::ui::gridHeader(grid);
        // Таблица — корень окна: после частей таблицы движок добавляет к корню свои слои, поэтому список ищется по модели
        // The table is the window root: after the table parts the engine adds its layers to the root, so the list is looked up by the model
        gran::ui::Node* found = nullptr;
        for (const auto& child : grid.children)
        {
            if (child->content.items == "totals")
                found = child.get();
        }
        TEST_ASSERT(found != nullptr);
        gran::ui::Node& pinned = *found;
        gran::ui::Node& scroll = *gran::ui::gridRows(grid)->parent;

        TEST_ASSERT(pinned.kind == gran::ui::ElementKind::STACK && pinned.children.size() == 1);
        gran::ui::Node& total = *pinned.children[0];
        TEST_ASSERT(cellText(total, 1) == "Итого" && cellText(total, 3) == "45");
        TEST_ASSERT(total.layout.frame.y >= scroll.layout.frame.y + scroll.layout.frame.height - 0.5f);
        wheel(driver, *rowsOf(grid)[0]->children[2], -1.0f, gran::platform::MODIFIER_SHIFT);
        TEST_ASSERT_NEAR(total.children[1]->layout.frame.x, header.children[1]->layout.frame.x, 0.5f);
        TEST_ASSERT_NEAR(total.children[0]->layout.frame.x, header.children[0]->layout.frame.x, 0.5f);
        gran::ui::moveGridColumn(grid, 1, 3);
        driver.settle();
        TEST_ASSERT(cellText(total, 3) == "Итого");
    }

    // Виртуализация столбцов: ячейки вне окна прокрутки не размещаются, End прокручивает к последнему / Column virtualization: the cells outside the scroll viewport are not placed, End scrolls to the last one
    void testColumnVirtualization()
    {
        std::string markup = R"(<Window width="400" height="300"><Resources><Data name="rows">
<Item c0="a" c11="z"/><Item c0="b" c11="y"/></Data><Selection name="sel" model="rows" mode="single"/></Resources>
<DataGrid id="g" items="rows" selection="sel" grow="1">)";
        for (int index = 0; index < 12; ++index)
            markup += "<GridColumn field=\"c" + std::to_string(index) + "\" width=\"100\"/>";
        markup += "</DataGrid></Window>";
        gran::Application app(headless());
        app.loadString(markup);
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        gran::ui::GridState& state = *grid.content.grid;
        gran::ui::Node& row = *rowsOf(grid)[0];

        TEST_ASSERT(state.view.shownFirst == 0 && state.view.shownEnd == 4);
        // Ячейка вне окна столбцов: рамка верная (её читают доступность и фокус, ревью Т-66), содержимое не разложено
        // A cell outside the column window: the frame is correct (accessibility and the focus read it, review T-66), the content is not laid out
        TEST_ASSERT(!gran::ui::gridCellShown(row, 11));
        TEST_ASSERT_NEAR(row.children[11]->layout.frame.x, row.children[0]->layout.frame.x + 1100.0f, 0.5f);
        TEST_ASSERT_NEAR(row.children[11]->layout.frame.width, 100.0f, 0.5f);
        TEST_ASSERT(row.children[11]->children[0]->layout.frame.width == 0.0f);
        driver.click(*row.children[0]);
        driver.press(gran::platform::Key::END);
        TEST_ASSERT(state.current == 11);
        TEST_ASSERT_NEAR(state.view.scrollX, 1200.0f - (grid.layout.frame.width - 2.0f), 1.0f);
        TEST_ASSERT(gran::ui::gridCellShown(row, 11) && state.view.shownFirst >= 7);
        TEST_ASSERT_NEAR(row.children[11]->layout.frame.width, 100.0f, 0.5f);
        TEST_ASSERT(!gran::ui::gridCellShown(row, 0));
    }
}

namespace suites
{
    void registerGridColumnTests(test::TestRunner& runner)
    {
        runner.add("grid columns: resize by edge", testResizeByEdge);
        runner.add("grid columns: edge double click", testEdgeDoubleClick);
        runner.add("grid columns: reorder by drag", testReorderByDrag);
        runner.add("grid columns: header keys", testHeaderKeys);
        runner.add("grid columns: permissions", testPermissions);
        runner.add("grid columns: layout attribute", testLayoutAttribute);
        runner.add("grid columns: layout settings", testLayoutSettings);
        runner.add("grid columns: damaged settings skipped", testDamagedSettingsSkipped);
        runner.add("grid columns: frozen scroll", testFrozenScroll);
        runner.add("grid columns: pinned rows", testPinnedRows);
        runner.add("grid columns: column virtualization", testColumnVirtualization);
    }
}
