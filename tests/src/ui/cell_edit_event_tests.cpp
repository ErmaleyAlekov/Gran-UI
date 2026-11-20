#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/data/list_model.hpp"
#include "gran/ui/data_grid.hpp"
#include "gran/ui/ui_event.hpp"
#include "gran/style/style_sheet.hpp"
#include "gran/capi/gran.h"
#include "gran/capi/internal/bridge.hpp"
#include <stdexcept>
#include <string>

namespace
{
    const char* const MARKUP = R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Column name="sum" type="number"/>
<Item name="Анна" sum="5"/><Item name="Борис" sum="30"/><Item name="Вера" sum="10"/></Data>
<Selection name="sel" model="rows"/></Resources>
<Stack><DataGrid id="g" items="rows" selection="sel" editable="true" onCellEdit="check" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="sum" width="100"/></DataGrid></Stack></Window>)";

    const char* const MARKUP_NO_HANDLER = R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Column name="sum" type="number"/>
<Item name="Анна" sum="5"/><Item name="Борис" sum="30"/><Item name="Вера" sum="10"/></Data>
<Selection name="sel" model="rows"/></Resources>
<Stack><DataGrid id="g" items="rows" selection="sel" editable="true" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="sum" width="100"/></DataGrid></Stack></Window>)";

    gran::AppOptions headless()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        return options;
    }

    gran::ui::Node& cellAt(gran::Application& app, std::size_t row, std::size_t column)
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

    gran::ui::Node* editorAt(gran::ui::Node& cell)
    {
        if (cell.children.empty() || cell.children.back()->kind != gran::ui::ElementKind::INPUT)
            return nullptr;
        return cell.children.back().get();
    }

    // Что увидел обработчик cellEdit / What the cellEdit handler saw
    struct Seen
    {
        gran::ui::EventType type = gran::ui::EventType::CUSTOM;
        gran::ui::ElementKind kind = gran::ui::ElementKind::BOX;
        std::size_t row = 0;
        std::string field;
        std::string oldValue;
        std::string value;
        int calls = 0;
        gran::ui::Node* current = nullptr;
    };

    // Правило обработчика в тесте / The handler rule in a test
    enum class Rule { OBSERVE, REFUSE_999, PREFIX_NAME, PREVENT, BAD_NUMBER };

    // Обработчик: запоминает событие и отвечает по правилу (копируется в onEvent — результат по указателю)
    // The handler: remembers the event and answers by the rule (copied into onEvent — the result is by pointer)
    struct CheckCell
    {
        Seen* seen;
        Rule rule;

        void operator()(gran::ui::Node& node, gran::ui::UiEvent& event) const
        {
            seen->type = event.type;
            seen->kind = event.target->kind;
            seen->row = event.cell.row;
            seen->field = event.cell.field;
            seen->oldValue = event.cell.oldValue;
            seen->value = event.cell.value;
            seen->calls++;
            seen->current = &node;
            if (rule == Rule::REFUSE_999 && event.cell.value == "999")
                event.cell.error = "Слишком много";
            if (rule == Rule::PREFIX_NAME && event.cell.field == "name")
                event.cell.value = "X" + event.cell.value;
            if (rule == Rule::PREVENT)
                event.preventDefault();
            if (rule == Rule::BAD_NUMBER)
                event.cell.value = "abc";
        }
    };

    void testCellEditEventFields()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        Seen seen;
        app.onEvent("check", CheckCell{&seen, Rule::OBSERVE});
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellAt(app, 1, 1));
        driver.press(gran::platform::Key::F2);
        driver.type("45");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(seen.calls == 1);
        TEST_ASSERT(seen.type == gran::ui::EventType::CELL_EDIT);
        TEST_ASSERT(seen.kind == gran::ui::ElementKind::GRID_CELL);
        TEST_ASSERT(seen.row == 1);
        TEST_ASSERT(seen.field == "sum");
        TEST_ASSERT(seen.oldValue == "30");
        TEST_ASSERT(seen.value == "45");
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "45");
        TEST_ASSERT(editorAt(cellAt(app, 1, 1)) == nullptr);
    }

    void testCellEditEventErrorRefuses()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        Seen seen;
        app.onEvent("check", CheckCell{&seen, Rule::REFUSE_999});
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellAt(app, 1, 1));
        driver.press(gran::platform::Key::F2);
        driver.type("999");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "30");
        TEST_ASSERT(editorAt(cellAt(app, 1, 1)) != nullptr);
        TEST_ASSERT(editorAt(cellAt(app, 1, 1))->content.tooltip == "Слишком много");
        TEST_ASSERT((editorAt(cellAt(app, 1, 1))->stateMask & gran::style::stateBit(gran::style::State::INVALID)) != 0);
        driver.press(gran::platform::Key::A, gran::platform::MODIFIER_CONTROL);
        driver.type("40");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "40");
        TEST_ASSERT(editorAt(cellAt(app, 1, 1)) == nullptr);
    }

    void testCellEditEventReplaceValue()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        Seen seen;
        app.onEvent("check", CheckCell{&seen, Rule::PREFIX_NAME});
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellAt(app, 1, 0));
        driver.press(gran::platform::Key::F2);
        driver.type("Глеб");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(1).at("name") == "XГлеб");
    }

    void testCellEditEventPreventDefault()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        Seen seen;
        app.onEvent("check", CheckCell{&seen, Rule::PREVENT});
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellAt(app, 1, 1));
        driver.press(gran::platform::Key::F2);
        driver.type("45");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "30");
        TEST_ASSERT(editorAt(cellAt(app, 1, 1)) != nullptr);
        driver.press(gran::platform::Key::ESCAPE);
        TEST_ASSERT(editorAt(cellAt(app, 1, 1)) == nullptr);
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "30");
    }

    void testCellEditEventReplacedValueCheckedByType()
    {
        gran::Application app(headless());
        app.loadString(MARKUP);
        Seen seen;
        app.onEvent("check", CheckCell{&seen, Rule::BAD_NUMBER});
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellAt(app, 1, 1));
        driver.press(gran::platform::Key::F2);
        driver.type("45");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "30");
        TEST_ASSERT(editorAt(cellAt(app, 1, 1)) != nullptr);
        TEST_ASSERT((editorAt(cellAt(app, 1, 1))->stateMask & gran::style::stateBit(gran::style::State::INVALID)) != 0);
    }

    void testCellEditEventListenerOnTheTable()
    {
        gran::Application app(headless());
        app.loadString(MARKUP_NO_HANDLER);
        Seen seen;
        app.addEventListener(*app.find("g"), "cellEdit", CheckCell{&seen, Rule::OBSERVE});
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellAt(app, 1, 1));
        driver.press(gran::platform::Key::F2);
        driver.type("45");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(seen.calls == 1);
        TEST_ASSERT(seen.current == app.find("g"));
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "45");
    }

    struct CState
    {
        std::string field;
        std::string old;
        std::string value;
        size_t row = 0;
        const gran_input_event* seen = nullptr;
    };

    int32_t answer(void* user, const gran_input_event* event)
    {
        CState* state = static_cast<CState*>(user);
        state->field = event->cell_field;
        state->old = event->cell_old;
        state->value = event->cell_value;
        state->row = event->cell_row;
        state->seen = event;
        gran_event_set_cell(event, "77", nullptr);
        return 0;
    }

    void testCellEditEventCInterface()
    {
        gran_app capp(headless());
        TEST_ASSERT(gran_app_load(&capp, MARKUP_NO_HANDLER, 0) == GRAN_OK);
        CState state;
        uint64_t id = 0;
        TEST_ASSERT(gran_app_listen(&capp, "g", "cellEdit", 0, answer, &state, &id) == GRAN_OK);
        gran::AppDriver driver(capp.application);
        driver.start();
        driver.click(cellAt(capp.application, 1, 1));
        driver.press(gran::platform::Key::F2);
        driver.type("45");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(state.row == 1);
        TEST_ASSERT(state.field == "sum");
        TEST_ASSERT(state.old == "30");
        TEST_ASSERT(state.value == "45");
        TEST_ASSERT(capp.application.model("rows").at(1).at("sum") == "77");
        TEST_ASSERT(gran_event_set_cell(state.seen, "1", nullptr) != GRAN_OK);
        TEST_ASSERT(gran_event_set_cell(nullptr, "1", nullptr) != GRAN_OK);
    }

    int32_t answerWithError(void* user, const gran_input_event* event)
    {
        CState* state = static_cast<CState*>(user);
        state->field = event->cell_field;
        state->old = event->cell_old;
        state->value = event->cell_value;
        state->row = event->cell_row;
        state->seen = event;
        gran_event_set_cell(event, nullptr, "нет");
        return 0;
    }

    void testCellEditEventCInterfaceError()
    {
        gran_app capp(headless());
        TEST_ASSERT(gran_app_load(&capp, MARKUP_NO_HANDLER, 0) == GRAN_OK);
        CState state;
        uint64_t id = 0;
        TEST_ASSERT(gran_app_listen(&capp, "g", "cellEdit", 0, answerWithError, &state, &id) == GRAN_OK);
        gran::AppDriver driver(capp.application);
        driver.start();
        driver.click(cellAt(capp.application, 1, 1));
        driver.press(gran::platform::Key::F2);
        driver.type("45");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(capp.application.model("rows").at(1).at("sum") == "30");
        TEST_ASSERT(editorAt(cellAt(capp.application, 1, 1)) != nullptr);
        TEST_ASSERT(editorAt(cellAt(capp.application, 1, 1))->content.tooltip == "нет");
    }
}

namespace
{
    // Т-74: Ctrl+Z вне редактора отменяет записанную правку, если у окна есть команда gran:undo (как в samples/grid.xml)
    // T-74: Ctrl+Z outside the editor undoes a written edit if the window has a gran:undo command (as in samples/grid.xml)
    void testCellEditUndoShortcut()
    {
        gran::Application app(headless());
        app.loadString(R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Column name="sum" type="number"/><Item name="Анна" sum="5"/><Item name="Борис" sum="30"/></Data>
<Selection name="sel" model="rows"/>
<Command id="undo" key="Mod+Z" action="gran:undo"/><Command id="redo" key="Mod+Y" action="gran:redo"/></Resources>
<Stack><DataGrid id="g" items="rows" selection="sel" editable="true" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="sum" width="100"/></DataGrid></Stack></Window>)");
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellAt(app, 1, 1));
        driver.press(gran::platform::Key::F2);
        driver.type("45");
        driver.press(gran::platform::Key::ENTER);
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "45");
        TEST_ASSERT(editorAt(cellAt(app, 1, 1)) == nullptr);
        driver.press(gran::platform::Key::Z, gran::platform::MODIFIER_CONTROL);
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "30");
        driver.press(gran::platform::Key::Y, gran::platform::MODIFIER_CONTROL);
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "45");
    }
}

namespace
{
    // Обработчик переводит фокус на другое поле и отказывает (ревью Т-75: повторный вход в окончание правки)
    // The handler moves the focus to another field and refuses (review T-75: re-entering the edit finish)
    struct FocusAndRefuse
    {
        gran::Application* app;
        int* calls;

        void operator()(gran::ui::Node&, gran::ui::UiEvent& event) const
        {
            ++*calls;
            app->focus(app->find("other"));
            event.cell.error = "нет";
        }
    };

    /*
    ** Смена фокуса внутри cellEdit только ставит отложенное окончание (pending), а не входит в finishGridEdit повторно.
    ** Ушедший фокус — окончание COMMIT: значение снова спрашивается, отказ отменяет правку. В модель ничего не пишется.
    **
    ** A focus change inside cellEdit only sets a deferred finish (pending) rather than re-entering finishGridEdit.
    ** The left focus is a COMMIT finish: the value is asked again, the refusal cancels the edit. Nothing is written into the model.
    */
    void testCellEditFocusAndRefuse()
    {
        gran::Application app(headless());
        app.loadString(R"(<Window width="500" height="300"><Resources>
<Data name="rows"><Column name="sum" type="number"/><Item name="Анна" sum="5"/><Item name="Борис" sum="30"/></Data>
<Selection name="sel" model="rows"/></Resources>
<Stack><Input id="other"/><DataGrid id="g" items="rows" selection="sel" editable="true" onCellEdit="check" height="200">
<GridColumn field="name" width="1fr"/><GridColumn field="sum" width="100"/></DataGrid></Stack></Window>)");
        int calls = 0;
        app.onEvent("check", FocusAndRefuse{&app, &calls});
        gran::AppDriver driver(app);
        driver.start();
        driver.click(cellAt(app, 1, 1));
        driver.press(gran::platform::Key::F2);
        driver.type("45");
        driver.press(gran::platform::Key::ENTER);
        driver.settle();
        TEST_ASSERT(calls >= 1);
        TEST_ASSERT(app.model("rows").at(1).at("sum") == "30");
        TEST_ASSERT(editorAt(cellAt(app, 1, 1)) == nullptr);
    }

    int32_t throwingAnswer(void* user, const gran_input_event* event)
    {
        static_cast<CState*>(user)->seen = event;
        throw std::runtime_error("callback");
    }

    // Callback бросил: текущая пара снимок-событие возвращена, старый снимок больше не принимается (ревью Т-75)
    // The callback threw: the current snapshot-event pair is restored, the old snapshot is no longer accepted (review T-75)
    void testCellEditCallbackThrows()
    {
        gran_app capp(headless());
        TEST_ASSERT(gran_app_load(&capp, MARKUP_NO_HANDLER, 0) == GRAN_OK);
        CState state;
        uint64_t id = 0;
        TEST_ASSERT(gran_app_listen(&capp, "g", "cellEdit", 0, throwingAnswer, &state, &id) == GRAN_OK);
        gran::AppDriver driver(capp.application);
        driver.start();
        driver.click(cellAt(capp.application, 1, 1));
        driver.press(gran::platform::Key::F2);
        driver.type("45");
        bool thrown = false;
        try
        {
            driver.press(gran::platform::Key::ENTER);
        }
        catch (const std::runtime_error&)
        {
            thrown = true;
        }
        TEST_ASSERT(thrown);
        TEST_ASSERT(state.seen != nullptr);
        TEST_ASSERT(gran_event_set_cell(state.seen, "1", nullptr) != GRAN_OK);
    }
}

namespace suites
{
    void registerCellEditEventTests(test::TestRunner& runner)
    {
        runner.add("cell edit event: fields", testCellEditEventFields);
        runner.add("cell edit event: error refuses", testCellEditEventErrorRefuses);
        runner.add("cell edit event: replace value", testCellEditEventReplaceValue);
        runner.add("cell edit event: prevent default", testCellEditEventPreventDefault);
        runner.add("cell edit event: replaced value checked by type", testCellEditEventReplacedValueCheckedByType);
        runner.add("cell edit event: listener on the table", testCellEditEventListenerOnTheTable);
        runner.add("cell edit event: C interface", testCellEditEventCInterface);
        runner.add("cell edit event: C interface error", testCellEditEventCInterfaceError);
        runner.add("cell edit event: Ctrl+Z after Enter", testCellEditUndoShortcut);
        runner.add("cell edit event: handler moves focus and refuses", testCellEditFocusAndRefuse);
        runner.add("cell edit event: C callback throws", testCellEditCallbackThrows);
    }
}
