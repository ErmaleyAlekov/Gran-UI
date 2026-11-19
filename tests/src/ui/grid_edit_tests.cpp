#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/data/list_model.hpp"
#include "gran/ui/data_grid.hpp"

#include <string>

// Правка ячеек DataGrid (пункт 2.2.4): основные пути — открыть, записать, отменить, уйти фокусом
// Editing DataGrid cells (item 2.2.4): the main paths — open, write, cancel, leave by the focus
namespace
{
    const char* const MARKUP = R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Column name="sum" type="number"/>
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

    // Ячейка column строки row списка строк таблицы g / The cell column of the row of the table g row list
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

    // Редактор — последний ребёнок ячейки, если он поле ввода / The editor is the last cell child if it is an input
    gran::ui::Node* editorIn(gran::ui::Node& cell)
    {
        if (cell.children.empty() || cell.children.back()->kind != gran::ui::ElementKind::INPUT)
            return nullptr;
        return cell.children.back().get();
    }

    // F2 открывает редактор с выделенным значением; набор заменяет его, Enter пишет в модель / F2 opens the editor with the value selected; typing replaces it, Enter writes into the model
    void testF2EnterWrites()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 1, 0));
        driver.press(gran::platform::Key::F2);
        gran::ui::Node* editor = editorIn(cellOf(app, 1, 0));
        TEST_ASSERT(editor != nullptr);
        TEST_ASSERT(editor->content.text == "Борис");
        TEST_ASSERT(editor->content.anchor == 0 && editor->content.caret == editor->content.text.size());
        TEST_ASSERT(!cellOf(app, 1, 0).children.front()->visible);
        driver.type("Глеб");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(1).at("name") == "Глеб");
        TEST_ASSERT(editorIn(cellOf(app, 1, 0)) == nullptr);
        TEST_ASSERT(cellOf(app, 1, 0).children.front()->visible);
        TEST_ASSERT(cellOf(app, 1, 0).children.front()->content.text == "Глеб");
        gran::ui::Node& rows = *gran::ui::gridRows(*app.find("g"));
        TEST_ASSERT((rows.stateMask & gran::style::stateBit(gran::style::State::FOCUSED)) != 0);
    }

    // Esc отменяет: модель не меняется, содержимое ячейки возвращается / Esc cancels: the model does not change, the cell content returns
    void testEscapeCancels()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 0));
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(editorIn(cellOf(app, 0, 0)) != nullptr);
        driver.type("xyz");
        driver.press(gran::platform::Key::ESCAPE);
        TEST_ASSERT(app.model("rows").at(0).at("name") == "Анна");
        TEST_ASSERT(editorIn(cellOf(app, 0, 0)) == nullptr);
        TEST_ASSERT(cellOf(app, 0, 0).children.front()->visible);
    }

    // Щелчок мимо (кнопка забирает фокус) — значение записывается / A click elsewhere (a button takes the focus) — the value is written
    void testFocusLossWrites()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 2, 1));
        driver.type("42");
        TEST_ASSERT(editorIn(cellOf(app, 2, 1)) != nullptr);
        TEST_ASSERT(editorIn(cellOf(app, 2, 1))->content.text == "42");
        driver.click(*app.find("other"));
        TEST_ASSERT(app.model("rows").at(2).at("sum") == "42");
        TEST_ASSERT(editorIn(cellOf(app, 2, 1)) == nullptr);
    }

    // Число с ошибкой: Enter не закрывает редактор (Invalid), Esc отменяет / A wrong number: Enter does not close the editor (Invalid), Esc cancels
    void testInvalidNumberStays()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 1));
        driver.type("12a");
        driver.press(gran::platform::Key::ENTER);
        gran::ui::Node* editor = editorIn(cellOf(app, 0, 1));
        TEST_ASSERT(editor != nullptr);
        TEST_ASSERT((editor->stateMask & gran::style::stateBit(gran::style::State::INVALID)) != 0);
        TEST_ASSERT(app.model("rows").at(0).at("sum") == "5");
        driver.press(gran::platform::Key::BACKSPACE);
        driver.type(",5");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(0).at("sum") == "12.5");
    }

    /*
    ** <EditTemplate> со списком: раскрытый список забирает фокус в свой слой — правка не должна закончиться;
    ** выбор пункта ставит значение, Enter записывает его в модель (значение — value пункта).
    **
    ** <EditTemplate> with a combo box: an open list takes the focus into its layer — the edit must not end;
    ** choosing an item sets the value, Enter writes it into the model (the value is the item value).
    */
    void testComboEditorPopup()
    {
        gran::Application app(headless());
        app.loadString(R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Item name="Анна" status="new"/><Item name="Борис" status="done"/></Data>
<Selection name="sel" model="rows"/></Resources>
<DataGrid id="g" items="rows" selection="sel" editable="true" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="status" width="150"><EditTemplate><ComboBox>
<MenuItem value="new" text="Новый"/><MenuItem value="done" text="Готов"/></ComboBox></EditTemplate></GridColumn>
</DataGrid></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 1));
        driver.press(gran::platform::Key::F2);
        gran::ui::Node& cell = cellOf(app, 0, 1);
        TEST_ASSERT(cell.children.back()->kind == gran::ui::ElementKind::COMBO_BOX);
        TEST_ASSERT(cell.children.back()->content.option == "new");
        driver.press(gran::platform::Key::DOWN);
        TEST_ASSERT(cell.children.back()->kind == gran::ui::ElementKind::COMBO_BOX);
        driver.press(gran::platform::Key::DOWN);
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(cell.children.back()->kind == gran::ui::ElementKind::COMBO_BOX);
        TEST_ASSERT(cell.children.back()->content.option == "done");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(0).at("status") == "done");
        TEST_ASSERT(cell.children.back()->kind != gran::ui::ElementKind::COMBO_BOX);
    }

    // Снимок 2.2.4: поле редактора не расширяет столбец auto своей шириной / Snapshot 2.2.4: the editor field does not widen an auto column by its own width
    void testEditorKeepsAutoWidth()
    {
        gran::Application app(headless());
        app.loadString(R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Item name="Ян" city="Уфа"/><Item name="Ия" city="Ош"/></Data>
<Selection name="sel" model="rows"/></Resources>
<DataGrid id="g" items="rows" selection="sel" editable="true" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="city" width="auto"/></DataGrid></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        float before = app.find("g")->content.grid->widths[1];
        driver.click(cellOf(app, 0, 1));
        driver.press(gran::platform::Key::F2);
        TEST_ASSERT(editorIn(cellOf(app, 0, 1)) != nullptr);
        TEST_ASSERT_NEAR(app.find("g")->content.grid->widths[1], before, 0.5f);
        driver.press(gran::platform::Key::ESCAPE);
        TEST_ASSERT_NEAR(app.find("g")->content.grid->widths[1], before, 0.5f);
    }

    // Tab в таблице-дереве: стрелка вправо там раскрывает ветку — переход к столбцу не должен на неё опираться
    // Tab in a tree table: the right arrow expands a branch there — moving to the column must not rely on it
    void testTabInTreeGrid()
    {
        gran::Application app(headless());
        app.loadString(R"(<Window width="400" height="300"><Resources>
<Tree name="org"><Item name="А" staff="3"><Item name="А1"/></Item><Item name="Б"/></Tree>
<TreeList name="rows" tree="org"/><Selection name="sel" model="rows" mode="single"/></Resources>
<DataGrid id="g" items="rows" selection="sel" editable="true" grow="1"><GridColumn field="name"/>
<GridColumn field="staff"/></DataGrid></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        gran::ui::Node& grid = *app.find("g");
        driver.click(cellOf(app, 0, 0));
        driver.press(gran::platform::Key::F2);
        driver.type("Я");
        driver.press(gran::platform::Key::TAB);
        TEST_ASSERT(app.treeList("rows").size() == 2);
        TEST_ASSERT(app.treeList("rows").at(0).at("name") == "Я");
        TEST_ASSERT(grid.content.grid->current == 1);
        TEST_ASSERT(editorIn(cellOf(app, 0, 1)) != nullptr);
        TEST_ASSERT(editorIn(cellOf(app, 0, 1))->content.text == "3");
        TEST_ASSERT((cellOf(app, 0, 1).stateMask & gran::style::stateBit(gran::style::State::CURRENT)) != 0);
        TEST_ASSERT((cellOf(app, 0, 0).stateMask & gran::style::stateBit(gran::style::State::CURRENT)) == 0);
    }

    // Ревью Т-69: закрытие окна (крестик, Alt+F4) записывает открытую правку до onClose / Review T-69: closing the window (the cross, Alt+F4) writes the open edit before onClose
    void testCloseWritesEdit()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 0));
        driver.type("Ада");
        TEST_ASSERT(editorIn(cellOf(app, 0, 0)) != nullptr);
        gran::platform::Event close;
        close.kind = gran::platform::EventKind::CLOSE;
        driver.window().pushEvent(close);
        driver.settle();
        TEST_ASSERT(app.model("rows").at(0).at("name") == "Ада");
    }

    // Ревью Т-69: фокус из кода между событиями — правка закрывается в безопасной точке до следующего события (F2) / Review T-69: the focus from code between the events — the edit closes at the safe point before the next event (F2)
    void testFocusFromCodeThenF2()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 0, 0));
        driver.type("Ия");
        app.focus(gran::ui::gridRows(*app.find("g")));
        driver.press(gran::platform::Key::F2);
        TEST_ASSERT(app.model("rows").at(0).at("name") == "Ия");
        TEST_ASSERT(editorIn(cellOf(app, 0, 0)) != nullptr);
        TEST_ASSERT(editorIn(cellOf(app, 0, 0))->content.text == "Ия");
    }

    // Ревью Т-69: приложение уничтожается с открытой правкой — наблюдатель записи отписывается до моделей (ASan) / Review T-69: the application is destroyed with an open edit — the record observer unsubscribes before the models (ASan)
    void testDestroyWithOpenEdit()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellOf(app, 1, 1));
        driver.type("9");
        TEST_ASSERT(editorIn(cellOf(app, 1, 1)) != nullptr);
    }
}

namespace suites
{
    void registerGridEditTests(test::TestRunner& runner)
    {
        runner.add("grid edit: F2, typing and Enter write the model", testF2EnterWrites);
        runner.add("grid edit: Escape cancels", testEscapeCancels);
        runner.add("grid edit: focus loss writes", testFocusLossWrites);
        runner.add("grid edit: invalid number keeps the editor", testInvalidNumberStays);
        runner.add("grid edit: combo box editor survives its open list", testComboEditorPopup);
        runner.add("grid edit: editor keeps an auto column width", testEditorKeepsAutoWidth);
        runner.add("grid edit: Tab in a tree table", testTabInTreeGrid);
        runner.add("grid edit: closing the window writes the edit", testCloseWritesEdit);
        runner.add("grid edit: focus from code, then F2", testFocusFromCodeThenF2);
        runner.add("grid edit: application destroyed with an open edit", testDestroyWithOpenEdit);
    }
}
