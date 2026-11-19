#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/data/list_model.hpp"
#include "gran/ui/data_grid.hpp"

#include <string>

// Правка ячеек DataGrid (пункт 2.2.4): дополнительные сценарии
// Editing DataGrid cells (item 2.2.4): additional scenarios
namespace
{
    const char* const MARKUP = R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Column name="name" type="text"/><Column name="sum" type="number"/>
<Item name="Анна" sum="5"/><Item name="Борис" sum="30"/><Item name="Вера" sum="10"/></Data>
<Selection name="sel" model="rows"/></Resources>
<Stack><DataGrid id="g" items="rows" selection="sel" editable="true" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="sum" width="100"/></DataGrid>
<Button id="other" text="Другое"/></Stack></Window>)";

    gran::AppOptions headless()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        return options;
    }

    gran::ui::Node& cellOf(gran::Application& app, std::size_t row, std::size_t column)
    {
        gran::ui::Node& rows = *gran::ui::gridRows(*app.find("g"));
        std::size_t seen = 0;
        for (const std::unique_ptr<gran::ui::Node>& child : rows.children)
        {
            if (child->kind != gran::ui::ElementKind::GRID_ROW)
                continue;
            if (seen++ == row)
                return *child->children[column];
        }
        return rows;
    }

    gran::ui::Node* editorIn(gran::ui::Node& cell)
    {
        if (cell.children.empty() || cell.children.back()->kind != gran::ui::ElementKind::INPUT)
            return nullptr;
        return cell.children.back().get();
    }

    // Tab: щелчок по ячейке (0,0), type("Ян"), press(Key::TAB) -> model("rows").at(0).at("name") == "Ян" и editorIn(cellOf(app,0,1)) != nullptr с text "5"; затем press(Key::TAB, gran::platform::MODIFIER_SHIFT) -> редактор снова в (0,0); press(Key::ESCAPE).
    // Tab: click on cell (0,0), type("Ян"), press(Key::TAB) -> model("rows").at(0).at("name") == "Ян" and editorIn(cellOf(app,0,1)) != nullptr with text "5"; then press(Key::TAB, gran::platform::MODIFIER_SHIFT) -> editor again in (0,0); press(Key::ESCAPE).
    void testTabNavigation()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 0));
        driver.type("Ян");
        driver.press(gran::platform::Key::TAB);
        TEST_ASSERT(app.model("rows").at(0).at("name") == "Ян");
        TEST_ASSERT(editorIn(cellOf(app, 0, 1)) != nullptr);
        TEST_ASSERT(editorIn(cellOf(app, 0, 1))->content.text == "5");
        driver.press(gran::platform::Key::TAB, gran::platform::MODIFIER_SHIFT);
        TEST_ASSERT(editorIn(cellOf(app, 0, 0)) != nullptr);
        driver.press(gran::platform::Key::ESCAPE);
    }

    // Отмена: щелчок (1,1), type("77"), ENTER -> sum == "77"; app.undo() -> "30"; app.redo() -> "77".
    // Cancel: click (1,1), type("77"), ENTER -> sum == "77"; app.undo() -> "30"; app.redo() -> "77".
    void testUndoRedo()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 1, 1));
        driver.type("77");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "77");
        app.undo();
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "30");
        app.redo();
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "77");
    }

    // Сортированный прокси: своя разметка, где в Resources после Data добавлен <SortFilter name="view" source="rows" sort="name"/> и <Selection name="sel" model="view"/>, а DataGrid items="view"; щелчок (0,0) (это 'Анна'), type("Яна"), ENTER -> app.model("rows").at(0).at("name") == "Яна" (запись в хранилище та же), затем app.undo() -> снова "Анна".
    // Sorted proxy: own markup, where in Resources after Data added <SortFilter name="view" source="rows" sort="name"/> and <Selection name="sel" model="view"/>, and DataGrid items="view"; click (0,0) (this is 'Анна'), type("Яна"), ENTER -> app.model("rows").at(0).at("name") == "Яна" (writing to the store is the same), then app.undo() -> again "Анна".
    void testSortedProxy()
    {
        const char* const SORTED_MARKUP = R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Column name="name" type="text"/><Column name="sum" type="number"/>
<Item name="Анна" sum="5"/><Item name="Борис" sum="30"/><Item name="Вера" sum="10"/></Data>
<SortFilter name="view" source="rows" sort="name"/>
<Selection name="sel" model="view"/></Resources>
<Stack><DataGrid id="g" items="view" selection="sel" editable="true" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="sum" width="100"/></DataGrid>
<Button id="other" text="Другое"/></Stack></Window>)";

        gran::Application app(headless());
        app.loadString(SORTED_MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 0));
        driver.type("Яна");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(0).at("name") == "Яна");
        app.undo();
        TEST_ASSERT(app.model("rows").at(0).at("name") == "Анна");
    }

    // Флажок: своя разметка, третий столбец <GridColumn field="done" type="boolean" width="80"/>; щелчок по ячейке (0,2), press(Key::F2) -> последний ребёнок ячейки имеет kind CHECKBOX; press(Key::SPACE); press(Key::ENTER) -> model done == "true".
    // Checkbox: own markup, third column <GridColumn field="done" type="boolean" width="80"/>; click on cell (0,2), press(Key::F2) -> the last child of the cell has kind CHECKBOX; press(Key::SPACE); press(Key::ENTER) -> model done == "true".
    void testCheckbox()
    {
        const char* const CHECKBOX_MARKUP = R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Column name="name" type="text"/><Column name="sum" type="number"/><Column name="done" type="boolean"/>
<Item name="Анна" sum="5" done="false"/><Item name="Борис" sum="30" done="false"/><Item name="Вера" sum="10" done="false"/></Data>
<Selection name="sel" model="rows"/></Resources>
<Stack><DataGrid id="g" items="rows" selection="sel" editable="true" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="sum" width="100"/><GridColumn field="done" type="boolean" width="80"/></DataGrid>
<Button id="other" text="Другое"/></Stack></Window>)";

        gran::Application app(headless());
        app.loadString(CHECKBOX_MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 2));
        driver.press(gran::platform::Key::F2);
        TEST_ASSERT(cellOf(app, 0, 2).children.back()->kind == gran::ui::ElementKind::CHECKBOX);
        driver.press(gran::platform::Key::SPACE);
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(0).at("done") == "true");
    }

    // Столбец с editable="false" (у столбца sum): щелчок (0,1), F2 -> editorIn == nullptr и модель не меняется.
    // Column with editable="false" (for column sum): click (0,1), F2 -> editorIn == nullptr and the model does not change.
    void testNonEditableColumn()
    {
        const char* const NON_EDITABLE_MARKUP = R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Column name="name" type="text"/><Column name="sum" type="number"/>
<Item name="Анна" sum="5"/><Item name="Борис" sum="30"/><Item name="Вера" sum="10"/></Data>
<Selection name="sel" model="rows"/></Resources>
<Stack><DataGrid id="g" items="rows" selection="sel" editable="true" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="sum" width="100" editable="false"/></DataGrid>
<Button id="other" text="Другое"/></Stack></Window>)";

        gran::Application app(headless());
        app.loadString(NON_EDITABLE_MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 1));
        driver.press(gran::platform::Key::F2);
        TEST_ASSERT(editorIn(cellOf(app, 0, 1)) == nullptr);
        TEST_ASSERT(app.model("rows").at(0).at("sum") == "5");
    }

    // Таблица без editable="true": щелчок (0,0), F2 и type("x") -> editorIn == nullptr, name == "Анна".
    // Table without editable="true": click (0,0), F2 and type("x") -> editorIn == nullptr, name == "Анна".
    void testNonEditableTable()
    {
        const char* const NON_EDITABLE_MARKUP = R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Column name="name" type="text"/><Column name="sum" type="number"/>
<Item name="Анна" sum="5"/><Item name="Борис" sum="30"/><Item name="Вера" sum="10"/></Data>
<Selection name="sel" model="rows"/></Resources>
<Stack><DataGrid id="g" items="rows" selection="sel" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="sum" width="100"/></DataGrid>
<Button id="other" text="Другое"/></Stack></Window>)";

        gran::Application app(headless());
        app.loadString(NON_EDITABLE_MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 0));
        driver.press(gran::platform::Key::F2);
        driver.type("x");
        TEST_ASSERT(editorIn(cellOf(app, 0, 0)) == nullptr);
        TEST_ASSERT(app.model("rows").at(0).at("name") == "Анна");
    }

    // Двойной щелчок: событие gran::platform::Event kind MOUSE_DOWN, position = driver.centerOf(ячейки (2,0)), button LEFT, clicks = 2, затем MOUSE_UP с той же точкой — каждое через driver.window().pushEvent и driver.settle(); до этого обычный driver.click по той же ячейке; -> editorIn(cellOf(app,2,0)) != nullptr.
    // Double click: event gran::platform::Event kind MOUSE_DOWN, position = driver.centerOf(cell (2,0)), button LEFT, clicks = 2, then MOUSE_UP with the same point — each through driver.window().pushEvent and driver.settle(); before that, ordinary driver.click on the same cell; -> editorIn(cellOf(app,2,0)) != nullptr.
    void testDoubleClick()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 2, 0));
        gran::platform::Event down;
        down.kind = gran::platform::EventKind::MOUSE_DOWN;
        down.position = driver.centerOf(cellOf(app, 2, 0));
        down.button = gran::platform::MouseButton::LEFT;
        down.clicks = 2;
        driver.window().pushEvent(down);
        driver.settle();
        gran::platform::Event up;
        up.kind = gran::platform::EventKind::MOUSE_UP;
        up.position = driver.centerOf(cellOf(app, 2, 0));
        up.button = gran::platform::MouseButton::LEFT;
        driver.window().pushEvent(up);
        driver.settle();
        TEST_ASSERT(editorIn(cellOf(app, 2, 0)) != nullptr);
    }

    // Запись удалена кодом во время правки: щелчок (1,0), type("Q"), app.model("rows").remove(1), driver.settle() -> app.model("rows").size() == 2, имена 'Анна' и 'Вера' не изменились (падения нет).
    // Record deleted by code during editing: click (1,0), type("Q"), app.model("rows").remove(1), driver.settle() -> app.model("rows").size() == 2, names 'Анна' and 'Вера' did not change (no crash).
    void testRecordDeletedDuringEdit()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 1, 0));
        driver.type("Q");
        app.model("rows").remove(1);
        driver.settle();
        TEST_ASSERT(app.model("rows").size() == 2);
        TEST_ASSERT(app.model("rows").at(0).at("name") == "Анна");
        TEST_ASSERT(app.model("rows").at(1).at("name") == "Вера");
    }

    // Пробел у таблицы не начинает правку: щелчок (0,0), type(" ") -> editorIn(cellOf(app,0,0)) == nullptr.
    // Space on the table does not start editing: click (0,0), type(" ") -> editorIn(cellOf(app,0,0)) == nullptr.
    void testSpaceDoesNotStartEdit()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 0));
        driver.type(" ");
        TEST_ASSERT(editorIn(cellOf(app, 0, 0)) == nullptr);
    }
}

namespace suites
{
    void registerGridEditMoreTests(test::TestRunner& runner)
    {
        runner.add("grid edit: Tab navigation", testTabNavigation);
        runner.add("grid edit: Undo and Redo", testUndoRedo);
        runner.add("grid edit: Sorted Proxy", testSortedProxy);
        runner.add("grid edit: Checkbox", testCheckbox);
        runner.add("grid edit: Non-editable Column", testNonEditableColumn);
        runner.add("grid edit: Non-editable Table", testNonEditableTable);
        runner.add("grid edit: Double Click", testDoubleClick);
        runner.add("grid edit: Record Deleted During Edit", testRecordDeletedDuringEdit);
        runner.add("grid edit: Space Does Not Start Edit", testSpaceDoesNotStartEdit);
    }
}
