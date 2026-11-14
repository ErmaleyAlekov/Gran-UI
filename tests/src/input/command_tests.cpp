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
    // Разметка с командами: окно, область focus (panel), поле, кнопки и меню / Markup with commands: the window, a focus area (panel), an input, buttons and a menu
    const char* const COMMANDS = R"(<Window><Resources>
        <Command id="saveDoc" text="Сохранить" key="Mod+S" action="save"/>
        <Command id="rename" text="Переименовать" key="F2"/>
        <Command id="del" key="Del" scope="focus" within="panel" action="delPanel"/>
        <Command id="delInner" key="Del" scope="focus" within="inner" action="delInner"/>
        <Command id="off" key="Mod+D" enabled="false"/>
        <Command id="all" key="Mod+G" scope="global"/>
      </Resources>
      <Stack>
        <Button id="save" command="saveDoc"/>
        <Input id="field"/>
        <Stack id="panel"><Button id="outer" text="o"/><Stack id="inner"><Button id="deep" text="d"/></Stack></Stack>
        <Button id="plain" text="p"/>
        <Popup id="menu"><MenuItem id="item" command="saveDoc"/><MenuItem id="other" text="x"/></Popup>
      </Stack></Window>)";

    // Получатель: запоминает сочетания и команды; shortcut выполняет поиск как приложение (одно окно)
    // A receiver: remembers the combinations and the commands; shortcut searches as the application does (one window)
    class CommandSink : public gran::input::ActionSink
    {
    public:
        void action(const std::string& name, gran::ui::Node&) override
        {
            actions.push_back(name);
        }

        bool shortcut(const gran::input::CommandQuery& query) override
        {
            std::vector<gran::input::CommandSource> sources{gran::input::CommandSource{commands, root, true}};
            gran::ui::Command* found = gran::input::matchCommand(sources, query);

            queries++;
            if (found != nullptr)
                ran.push_back(found->id);
            return found != nullptr;
        }

        void command(const std::string& id, gran::ui::Node& node) override
        {
            ran.push_back(id + "@" + node.id);
        }

        std::vector<gran::ui::Command>* commands = nullptr;
        gran::ui::Node* root = nullptr;
        std::vector<std::string> actions;
        std::vector<std::string> ran;
        int queries = 0;
    };

    // Раскладка текста не нужна: каретка остаётся на месте / The text layout is not needed: the caret stays in place
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

    // Документ с компоновкой и маршрутизатор над ним / A laid out document and a router over it
    struct Fixture
    {
        gran::markup::UiDocument doc = support::layoutMarkup(COMMANDS, 400, 300);
        CommandSink sink;
        NoTextLocator locator;
        gran::input::InputRouter router{*doc.root, sink, nullptr, locator};

        Fixture()
        {
            sink.commands = &doc.commands;
            sink.root = doc.root.get();
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
    };

    const std::uint8_t CTRL = gran::platform::MODIFIER_CONTROL;

    // Подпись «Mod+буква» по соглашению ОС: «⌘S» на macOS, «Ctrl+S» на Windows и Linux (первый прогон CI на macOS)
    // The "Mod+letter" label by the OS convention: "⌘S" on macOS, "Ctrl+S" on Windows and Linux (the first CI run on macOS)
    std::string modLabel(const std::string& letter)
    {
        if (gran::platform::hostOs() == gran::platform::HostOs::MACOS)
            return "⌘" + letter;
        return "Ctrl+" + letter;
    }

    void testMarkupBindsCommands()
    {
        Fixture f;

        TEST_ASSERT(f.doc.commands.size() == static_cast<std::size_t>(6));
        TEST_ASSERT(f.node("save").content.text == "Сохранить");
        TEST_ASSERT(f.node("save").content.shortcut == modLabel("S"));
        TEST_ASSERT(f.node("item").content.text == "Сохранить");
        TEST_ASSERT(f.doc.commands[2].scope == gran::ui::CommandScope::FOCUS);
        TEST_ASSERT(!f.doc.commands[4].enabled);
    }

    void testMarkupRejectsBadCommands()
    {
        gran::markup::LoadOptions options;

        TEST_ASSERT_THROWS(gran::markup::loadMarkupString(
            R"(<Window><Resources><Command id="a" key="Mod+Foo"/></Resources><Box/></Window>)", options),
            gran::MarkupException);
        TEST_ASSERT_THROWS(gran::markup::loadMarkupString(
            R"(<Window><Resources><Command id="a" scope="focus"/></Resources><Box/></Window>)", options),
            gran::MarkupException);
        TEST_ASSERT_THROWS(gran::markup::loadMarkupString(
            R"(<Window><Resources><Command id="a"/></Resources><Button command="a" enabled="false"/></Window>)", options),
            gran::MarkupException);
    }

    void testWindowShortcut()
    {
        Fixture f;

        f.press(gran::platform::Key::S, CTRL);
        f.press(gran::platform::Key::D, CTRL);
        f.press(gran::platform::Key::G, CTRL);
        TEST_ASSERT((f.sink.ran == std::vector<std::string>{"saveDoc", "all"}));
    }

    void testFieldKeepsEditingKeys()
    {
        Fixture f;

        f.router.focus(&f.node("field"));
        f.press(gran::platform::Key::DELETE_KEY, 0);
        f.press(gran::platform::Key::A, CTRL);
        f.press(gran::platform::Key::S, 0);
        TEST_ASSERT(f.sink.queries == 0);
        f.press(gran::platform::Key::S, CTRL);
        f.press(gran::platform::Key::F2, 0);
        TEST_ASSERT((f.sink.ran == std::vector<std::string>{"saveDoc", "rename"}));
    }

    void testFocusScopeDeepestFirst()
    {
        Fixture f;

        f.router.focus(&f.node("deep"));
        f.press(gran::platform::Key::DELETE_KEY, 0);
        f.router.focus(&f.node("outer"));
        f.press(gran::platform::Key::DELETE_KEY, 0);
        f.router.focus(&f.node("plain"));
        f.press(gran::platform::Key::DELETE_KEY, 0);
        TEST_ASSERT((f.sink.ran == std::vector<std::string>{"delInner", "del"}));
    }

    void testControlAndMenuKeepKeys()
    {
        Fixture f;

        f.router.focus(&f.node("plain"));
        f.press(gran::platform::Key::SPACE, 0);
        TEST_ASSERT(f.sink.queries == 0);
        f.router.openPopup(f.node("menu"), gran::Rect{0.0f, 0.0f, 10.0f, 10.0f}, false);
        f.press(gran::platform::Key::S, CTRL);
        TEST_ASSERT(f.sink.queries == 0);
    }

    void testActivationRunsCommand()
    {
        Fixture f;

        f.router.focus(&f.node("save"));
        f.press(gran::platform::Key::ENTER, 0);
        TEST_ASSERT((f.sink.ran == std::vector<std::string>{"saveDoc@save"}));
    }

    // Модальный диалог пропускает только команды focus внутри себя / A modal dialog lets through only the focus commands inside it
    void testModalBlocksWindowCommands()
    {
        Fixture f;
        gran::input::CommandQuery query;
        std::vector<gran::input::CommandSource> sources{gran::input::CommandSource{&f.doc.commands, f.doc.root.get(), true}};

        query.pressed.key = gran::platform::Key::S;
        query.pressed.modifiers = CTRL;
        query.modal = &f.node("menu");
        TEST_ASSERT(gran::input::matchCommand(sources, query) == nullptr);
        query.modal = nullptr;
        TEST_ASSERT(gran::input::matchCommand(sources, query) == &f.doc.commands[0]);
        sources[0].own = false;
        TEST_ASSERT(gran::input::matchCommand(sources, query) == nullptr);
        query.pressed.key = gran::platform::Key::G;
        TEST_ASSERT(gran::input::matchCommand(sources, query) == &f.doc.commands[5]);
    }

    // Обработчик действия пишет «имя@id узла» в журнал / An action handler writes "name@node id" into the log
    struct Recorder
    {
        std::vector<std::string>* log;
        std::string name;

        void operator()(gran::ui::Node& node) const
        {
            log->push_back(name + "@" + node.id);
        }
    };

    const char* const APP_MARKUP = R"(<Window><Resources><Command id="saveDoc" text="Сохранить" key="Mod+S" action="save"/></Resources>
        <Stack id="root"><Button id="b" command="saveDoc"/></Stack></Window>)";

    void testApplicationCommandState()
    {
        gran::Application app;
        std::vector<std::string> log;

        app.loadString(APP_MARKUP);
        app.onAction("save", Recorder{&log, "save"});
        TEST_ASSERT(app.runCommand("saveDoc"));
        TEST_ASSERT((log == std::vector<std::string>{"save@root"}));
        app.setCommandEnabled("saveDoc", false);
        TEST_ASSERT(!app.find("b")->enabled);
        TEST_ASSERT(!app.commandEnabled("saveDoc"));
        TEST_ASSERT(!app.runCommand("saveDoc"));
        TEST_ASSERT(!app.runCommand("missing"));
        app.loadString(APP_MARKUP);
        TEST_ASSERT(!app.find("b")->enabled);
        gran::app::executeCommand(app, "enableCommand", "saveDoc", "");
        TEST_ASSERT(app.find("b")->enabled);
        TEST_ASSERT(app.commandEnabled("saveDoc"));
        TEST_ASSERT(log.size() == static_cast<std::size_t>(1));
    }

    void testCommandInListTemplate()
    {
        gran::Application app;

        app.loadString(R"(<Window><Resources><Data name="rows"><Item n="1"/><Item n="2"/></Data>
            <Command id="del" text="Удалить"/></Resources>
            <Stack id="list" items="rows"><ItemTemplate><Button command="del"/></ItemTemplate></Stack></Window>)");
        gran::ui::Node& list = *app.find("list");
        TEST_ASSERT(list.children.size() == static_cast<std::size_t>(2));
        TEST_ASSERT(list.children[0]->content.text == "Удалить");
        app.setCommandEnabled("del", false);
        TEST_ASSERT(!list.children[1]->enabled);
        app.model("rows").append(gran::data::Record{{"n", "3"}});
        TEST_ASSERT(list.children.size() == static_cast<std::size_t>(3));
        TEST_ASSERT(!list.children[2]->enabled);
        TEST_ASSERT(list.children[2]->content.text == "Удалить");
    }

    void testGlobalCommandInOtherWindow()
    {
        gran::Application app;

        app.loadString(R"(<Window><Resources><Command id="all" text="Всё" key="Mod+G" scope="global"/>
            <Command id="mine" text="Своя"/></Resources><Box/></Window>)");
        app.openWindowString(R"(<Window><Stack><Button id="tb" command="all"/><Button id="tm" text="m" command="mine"/></Stack></Window>)");
        TEST_ASSERT(app.find("tb")->content.text == "Всё");
        TEST_ASSERT(app.find("tb")->content.shortcut == modLabel("G"));
        TEST_ASSERT(app.find("tm")->content.shortcut.empty());
        app.setCommandEnabled("all", false);
        TEST_ASSERT(!app.find("tb")->enabled);
    }
}

namespace suites
{
    void registerCommandTests(test::TestRunner& runner)
    {
        runner.add("command: markup binds buttons and menu items", testMarkupBindsCommands);
        runner.add("command: markup rejects bad commands", testMarkupRejectsBadCommands);
        runner.add("command: window and global shortcuts", testWindowShortcut);
        runner.add("command: input keeps editing keys", testFieldKeepsEditingKeys);
        runner.add("command: focus scope deepest first", testFocusScopeDeepestFirst);
        runner.add("command: controls and menus keep keys", testControlAndMenuKeepKeys);
        runner.add("command: activation runs command", testActivationRunsCommand);
        runner.add("command: modal blocks window commands", testModalBlocksWindowCommands);
        runner.add("command: application state and reload", testApplicationCommandState);
        runner.add("command: list template copies", testCommandInListTemplate);
        runner.add("command: global command in another window", testGlobalCommandInOtherWindow);
    }
}
