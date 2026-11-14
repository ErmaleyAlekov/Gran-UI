#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/commands.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/input/input_router.hpp"
#include "gran/platform/host.hpp"
#include "gran/markup/markup_loader.hpp"
#include "support/layout_helpers.hpp"

#include <string>
#include <vector>

namespace
{
    const char* const LAYOUT_MARKUP = R"(<Window><Resources><Command id="tool" text="Отменить"/></Resources><Stack>
        <Input id="field" onChange="changed"/>
        <Input id="plain" undo="false"/>
        <Button id="toolButton" command="tool"/>
        <Button id="menuButton" text="Правка" popup="editMenu"/>
        <Button id="plainButton" text="Кнопка"/>
        <Popup id="editMenu"><MenuItem id="undoItem" text="Отменить"/></Popup>
        <TextArea id="area"/>
        <Input id="locked" readOnly="true" value="x"/>
    </Stack></Window>)";

    class UndoSink : public gran::input::ActionSink
    {
    public:
        void action(const std::string& name, gran::ui::Node&) override
        {
            actions.push_back(name);
        }

        bool shortcut(const gran::input::CommandQuery&) override
        {
            return false;
        }

        void command(const std::string&, gran::ui::Node&) override
        {
        }

        void editingChanged() override
        {
            ++editing;
        }

        std::vector<std::string> actions;
        int editing = 0;
    };

    class NoTextLocator : public gran::input::TextLocator
    {
    public:
        std::size_t offsetAt(const gran::ui::Node&, gran::Vec2) override
        {
            return 0;
        }

        std::size_t lineEdge(const gran::ui::Node&, std::size_t offset, bool) override
        {
            return offset;
        }

        std::size_t lineMove(const gran::ui::Node&, std::size_t offset, int, float&) override
        {
            return offset;
        }

        int pageLines(const gran::ui::Node&) override
        {
            return 1;
        }
    };

    struct Fixture
    {
        gran::markup::UiDocument doc = support::layoutMarkup(LAYOUT_MARKUP, 400, 300);
        UndoSink sink;
        NoTextLocator locator;
        gran::input::InputRouter router{*doc.root, sink, nullptr, locator};

        Fixture()
        {
            router.focus(&node("field"));
        }

        gran::ui::Node& node(const char* id)
        {
            return *gran::ui::findNode(*doc.root, id);
        }

        void press(gran::platform::Key key, std::uint8_t modifiers)
        {
            gran::platform::Event event;
            event.kind = gran::platform::EventKind::KEY_DOWN;
            event.key = key;
            event.modifiers = modifiers;
            router.handle(event);
        }

        // Щелчок левой кнопкой в центре узла / A left click in the node center
        void click(const char* id)
        {
            const gran::Rect& frame = node(id).layout.frame;
            gran::platform::Event event;

            event.kind = gran::platform::EventKind::MOUSE_DOWN;
            event.button = gran::platform::MouseButton::LEFT;
            event.position = {frame.x + frame.width / 2, frame.y + frame.height / 2};
            router.handle(event);
            event.kind = gran::platform::EventKind::MOUSE_UP;
            router.handle(event);
        }

        void type(gran::input::InputRouter& target, const std::string& text)
        {
            for (char c : text)
            {
                gran::platform::Event event;
                event.kind = gran::platform::EventKind::CHARACTER;
                event.character = static_cast<char32_t>(c);
                target.handle(event);
            }
        }
    };

    const std::uint8_t CTRL = gran::platform::MODIFIER_CONTROL;
    const std::uint8_t SHIFT = gran::platform::MODIFIER_SHIFT;

    void testTextUndo()
    {
        Fixture f;

        f.type(f.router, "abc");
        f.press(gran::platform::Key::Z, CTRL);
        TEST_ASSERT(f.node("field").content.text.empty());
        TEST_ASSERT(f.node("field").content.caret == 0);
        // Повтор: Mod+Y — Windows и Linux; на macOS Cmd+Y не повтор (Т-46), там Mod+Shift+Z
        // Redo: Mod+Y — Windows and Linux; on macOS Cmd+Y is not redo (T-46), there it is Mod+Shift+Z
        f.press(gran::platform::Key::Y, CTRL);
        if (gran::platform::hostOs() == gran::platform::HostOs::MACOS)
        {
            TEST_ASSERT(f.node("field").content.text.empty());
            f.press(gran::platform::Key::Z, CTRL | SHIFT);
        }
        TEST_ASSERT(f.node("field").content.text == "abc");
        TEST_ASSERT(f.node("field").content.caret == 3);
        f.press(gran::platform::Key::Z, CTRL);
        f.press(gran::platform::Key::Z, CTRL | SHIFT);
        TEST_ASSERT(f.node("field").content.text == "abc");
    }

    void testTextUndoWithMove()
    {
        Fixture f;

        f.type(f.router, "ab");
        f.press(gran::platform::Key::LEFT, 0);
        f.type(f.router, "c");
        TEST_ASSERT(f.node("field").content.text == "acb");
        f.press(gran::platform::Key::Z, CTRL);
        TEST_ASSERT(f.node("field").content.text == "ab");
        f.press(gran::platform::Key::Z, CTRL);
        TEST_ASSERT(f.node("field").content.text.empty());
    }

    void testTextUndoWithBackspace()
    {
        Fixture f;

        f.type(f.router, "abcd");
        f.press(gran::platform::Key::BACKSPACE, 0);
        f.press(gran::platform::Key::BACKSPACE, 0);
        TEST_ASSERT(f.node("field").content.text == "ab");
        f.press(gran::platform::Key::Z, CTRL);
        TEST_ASSERT(f.node("field").content.text == "abcd");
    }

    void testTextUndoWithSelectAll()
    {
        Fixture f;

        f.type(f.router, "hello");
        f.press(gran::platform::Key::A, CTRL);
        f.type(f.router, "x");
        TEST_ASSERT(f.node("field").content.text == "x");
        f.press(gran::platform::Key::Z, CTRL);
        TEST_ASSERT(f.node("field").content.text == "hello");
        TEST_ASSERT(f.node("field").content.anchor == 0);
        TEST_ASSERT(f.node("field").content.caret == 5);
    }

    void testTextUndoWithOnChange()
    {
        Fixture f;

        f.type(f.router, "abc");
        // Набор сам вызывает onChange; проверяется вызов от отмены / Typing calls onChange itself; the call from undo is checked
        TEST_ASSERT(!f.sink.actions.empty());
        f.sink.actions.clear();
        f.press(gran::platform::Key::Z, CTRL);
        TEST_ASSERT(!f.sink.actions.empty());
        TEST_ASSERT(f.sink.actions.back() == "changed");
        f.sink.actions.clear();
    }

    void testTextUndoWithProgrammaticChange()
    {
        Fixture f;

        f.type(f.router, "ab");
        f.node("field").content.text = "zzz";
        f.node("field").content.caret = 3;
        f.node("field").content.anchor = 3;
        f.press(gran::platform::Key::Z, CTRL);
        TEST_ASSERT(f.node("field").content.text == "zzz");
        TEST_ASSERT(f.node("field").content.caret == 3);
        TEST_ASSERT(f.node("field").content.anchor == 3);
    }

    void testTextAreaUndo()
    {
        Fixture f;

        f.router.focus(&f.node("area"));
        f.type(f.router, "ab");
        TEST_ASSERT(f.node("area").content.text == "ab");
        f.press(gran::platform::Key::Z, CTRL);
        TEST_ASSERT(f.node("area").content.text.empty());
    }

    // undo="false": правки не пишутся в историю / undo="false": the edits are not written to the history
    void testUndoDisabled()
    {
        Fixture f;

        f.router.focus(&f.node("plain"));
        f.type(f.router, "ab");
        f.press(gran::platform::Key::Z, CTRL);
        TEST_ASSERT(f.node("plain").content.text == "ab");
        TEST_ASSERT(f.node("plain").content.history == nullptr);
    }

    // Отмена поля из кода приложения (gran:undo) и оповещение о смене истории / Undoing the field from the application code (gran:undo) and the history change notice
    void testFieldUndoFromRouter()
    {
        Fixture f;

        TEST_ASSERT(!f.router.canUndoField(false));
        f.sink.editing = 0;
        f.type(f.router, "ab");
        TEST_ASSERT(f.sink.editing > 0);
        TEST_ASSERT(f.router.canUndoField(false));
        TEST_ASSERT(!f.router.canUndoField(true));
        TEST_ASSERT(f.router.undoField(false));
        TEST_ASSERT(f.node("field").content.text.empty());
        TEST_ASSERT(f.router.canUndoField(true));
        TEST_ASSERT(f.router.undoField(true));
        TEST_ASSERT(f.node("field").content.text == "ab");
        TEST_ASSERT(!f.router.undoField(true));
    }

    // Кнопка команды и кнопка меню фокус у поля не забирают, обычная кнопка — забирает / A command button and a menu button keep the field focus, a plain button takes it
    void testCommandButtonKeepsField()
    {
        Fixture f;

        f.type(f.router, "ab");
        f.click("toolButton");
        TEST_ASSERT(f.router.focused() == &f.node("field"));
        f.click("menuButton");
        TEST_ASSERT(f.node("editMenu").visible);
        // Фокус в открытом меню: редактируемым остаётся поле / The focus in the open menu: the field stays the edited one
        TEST_ASSERT(f.router.canUndoField(false));
        f.press(gran::platform::Key::ESCAPE, 0);
        TEST_ASSERT(f.router.focused() == &f.node("field"));
        f.click("plainButton");
        TEST_ASSERT(f.router.focused() == &f.node("plainButton"));
        TEST_ASSERT(!f.router.canUndoField(false));
    }

    void testReadOnlyUndo()
    {
        Fixture f;

        f.router.focus(&f.node("locked"));
        f.type(f.router, "y");
        f.press(gran::platform::Key::Z, CTRL);
        TEST_ASSERT(f.node("locked").content.text == "x");
    }
}

namespace suites
{
    void registerTextUndoTests(test::TestRunner& runner)
    {
        runner.add("text undo: basic undo/redo", testTextUndo);
        runner.add("text undo: undo with move", testTextUndoWithMove);
        runner.add("text undo: undo with backspace", testTextUndoWithBackspace);
        runner.add("text undo: undo with select all", testTextUndoWithSelectAll);
        runner.add("text undo: undo with onChange", testTextUndoWithOnChange);
        runner.add("text undo: undo with programmatic change", testTextUndoWithProgrammaticChange);
        runner.add("text undo: undo in TextArea", testTextAreaUndo);
        runner.add("text undo: undo in readOnly field", testReadOnlyUndo);
        runner.add("text undo: undo=\"false\"", testUndoDisabled);
        runner.add("text undo: field undo for gran:undo", testFieldUndoFromRouter);
        runner.add("text undo: command buttons keep the field focus", testCommandButtonKeepsField);
    }
}
