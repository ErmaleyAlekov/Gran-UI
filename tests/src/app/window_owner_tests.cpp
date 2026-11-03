#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/commands.hpp"
#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/markup/markup_loader.hpp"

#include <string>
#include <vector>

namespace
{
    // Окно в собственном состоянии теста (без окна ОС) / A window in the test own state (without an OS window)
    gran::app::AppWindow& addTestWindow(gran::Application::State& state, const char* id)
    {
        std::string markup = std::string("<Window><Stack><Text id=\"") + id + "\">x</Text></Stack></Window>";

        return gran::app::addWindow(state, gran::markup::loadMarkupString(markup, gran::markup::LoadOptions{}), ".");
    }

    // Подчинённое окно owner; modal — модальное / An owned window of owner; modal — a modal one
    gran::app::AppWindow& addOwned(gran::Application::State& state, const char* id, gran::app::AppWindow& owner,
        bool modal)
    {
        gran::app::AppWindow& win = addTestWindow(state, id);
        gran::WindowOptions options;

        options.owner = owner.document.root.get();
        options.modal = modal;
        gran::app::attachOwner(win, gran::app::requireOwner(state, options), options);
        return win;
    }

    // Записывает имя окна и результат закрытия / Records the window name and the close result
    struct CloseLog
    {
        std::vector<std::string>* log;
        std::string name;

        void operator()(const std::string& result) const
        {
            log->push_back(name + "=" + result);
        }
    };

    void testOwnerLookup()
    {
        gran::Application::State state;
        gran::app::AppWindow& main = addTestWindow(state, "m");
        gran::WindowOptions options;
        gran::markup::UiDocument foreign = gran::markup::loadMarkupString("<Window><Text>y</Text></Window>",
            gran::markup::LoadOptions{});

        TEST_ASSERT(gran::app::requireOwner(state, options) == nullptr);
        options.owner = main.document.root->children.front().get();
        TEST_ASSERT(gran::app::requireOwner(state, options) == &main);
        options.owner = foreign.root.get();
        TEST_ASSERT_THROWS(gran::app::requireOwner(state, options), gran::GranException);
    }

    void testModalNeedsOwner()
    {
        gran::Application::State state;
        gran::app::AppWindow& win = addTestWindow(state, "m");
        gran::WindowOptions options;

        options.modal = true;
        gran::app::attachOwner(win, nullptr, options);
        TEST_ASSERT(!win.modal);
    }

    void testBlockingChain()
    {
        gran::Application::State state;
        gran::app::AppWindow& main = addTestWindow(state, "m");
        gran::app::AppWindow& tool = addOwned(state, "t", main, false);

        TEST_ASSERT(gran::app::blockingModal(state, main) == nullptr);
        gran::app::AppWindow& dialog = addOwned(state, "d", main, true);
        TEST_ASSERT(gran::app::blockingModal(state, main) == &dialog);
        TEST_ASSERT(gran::app::blockingModal(state, dialog) == nullptr);
        TEST_ASSERT(gran::app::blockingModal(state, tool) == nullptr);
        gran::app::AppWindow& nested = addOwned(state, "n", dialog, true);
        TEST_ASSERT(gran::app::blockingModal(state, main) == &nested);
        TEST_ASSERT(gran::app::blockingModal(state, dialog) == &nested);
        gran::app::updateBlocking(state);
        TEST_ASSERT(main.blocked && dialog.blocked && !nested.blocked && !tool.blocked);
    }

    void testBlockedInputDropped()
    {
        gran::Application::State state;
        gran::app::AppWindow& main = addTestWindow(state, "m");
        gran::app::AppWindow& dialog = addOwned(state, "d", main, true);
        gran::platform::Event press;
        gran::platform::Event resize;

        press.kind = gran::platform::EventKind::MOUSE_DOWN;
        resize.kind = gran::platform::EventKind::RESIZE;
        gran::app::updateBlocking(state);
        TEST_ASSERT(gran::app::dropBlockedInput(main, press));
        TEST_ASSERT(!gran::app::dropBlockedInput(main, resize));
        TEST_ASSERT(!gran::app::dropBlockedInput(dialog, press));
    }

    void testCloseWithResult()
    {
        gran::Application::State state;
        gran::app::AppWindow& main = addTestWindow(state, "m");
        gran::app::AppWindow& dialog = addOwned(state, "d", main, true);
        std::vector<std::string> log;

        dialog.onClosed = CloseLog{&log, "dialog"};
        gran::app::updateBlocking(state);
        gran::app::markClosing(state, *dialog.document.root, "ok");
        gran::app::closeWindows(state);
        TEST_ASSERT(state.windows.size() == 1);
        TEST_ASSERT((log == std::vector<std::string>{"dialog=ok"}));
        TEST_ASSERT(!main.blocked);
    }

    void testCascadeOrder()
    {
        gran::Application::State state;
        gran::app::AppWindow& main = addTestWindow(state, "m");
        gran::app::AppWindow& dialog = addOwned(state, "d", main, false);
        gran::app::AppWindow& tool = addOwned(state, "t", dialog, false);
        std::vector<std::string> log;

        dialog.onClosed = CloseLog{&log, "dialog"};
        tool.onClosed = CloseLog{&log, "tool"};
        tool.closeResult = "stale";
        gran::app::markClosing(state, *dialog.document.root, "ok");
        gran::app::closeWindows(state);
        TEST_ASSERT((log == std::vector<std::string>{"tool=", "dialog=ok"}));
        TEST_ASSERT(state.windows.size() == 1 && !state.quit);
    }

    void testMainCloseQuits()
    {
        gran::Application::State state;
        gran::app::AppWindow& main = addTestWindow(state, "m");
        gran::app::AppWindow& dialog = addOwned(state, "d", main, true);
        std::vector<std::string> log;

        dialog.onClosed = CloseLog{&log, "dialog"};
        main.closing = true;
        gran::app::closeWindows(state);
        TEST_ASSERT(state.quit);
        TEST_ASSERT(log.empty());
        TEST_ASSERT(state.windows.size() == 2);
    }

    void testBuiltinCloseResult()
    {
        gran::Application app;
        gran::Application::State state;
        state.owner = &app;
        gran::app::AppWindow& main = addTestWindow(state, "m");
        gran::app::AppWindow& dialog = addOwned(state, "d", main, true);

        TEST_ASSERT(gran::app::executeBuiltin(state, "gran:close:ok", *dialog.document.root->children.front()));
        TEST_ASSERT(dialog.closing && dialog.closeResult == "ok");
        gran::app::markClosing(state, *dialog.document.root, "cancel");
        TEST_ASSERT(dialog.closeResult == "cancel");
        TEST_ASSERT(!main.closing);
    }

    void testWindowKindMarkup()
    {
        gran::markup::UiDocument document = gran::markup::loadMarkupString(
            "<Window kind=\"dialog\" position=\"centerOwner\"><Text>x</Text></Window>", gran::markup::LoadOptions{});
        gran::Application::State state;
        gran::app::AppWindow& win = addTestWindow(state, "m");
        gran::platform::WindowDesc desc;

        TEST_ASSERT(document.window.kind == gran::platform::WindowKind::DIALOG);
        TEST_ASSERT(document.window.position == gran::platform::WindowPosition::CENTER_OWNER);
        TEST_ASSERT_THROWS(gran::markup::loadMarkupString("<Window kind=\"popup\"><Text>x</Text></Window>",
            gran::markup::LoadOptions{}), gran::GranException);
        gran::app::ownWindow(win, desc);
        TEST_ASSERT(desc.kind == gran::platform::WindowKind::NORMAL && desc.owner == nullptr);
        TEST_ASSERT(desc.position == gran::platform::WindowPosition::DEFAULT);
    }
}

namespace suites
{
    void registerWindowOwnerTests(test::TestRunner& runner)
    {
        runner.add("window owner: lookup by node", testOwnerLookup);
        runner.add("window owner: modal needs an owner", testModalNeedsOwner);
        runner.add("window owner: blocking chain", testBlockingChain);
        runner.add("window owner: blocked input dropped", testBlockedInputDropped);
        runner.add("window owner: close with result", testCloseWithResult);
        runner.add("window owner: owned close first", testCascadeOrder);
        runner.add("window owner: main close quits", testMainCloseQuits);
        runner.add("window owner: gran:close:result", testBuiltinCloseResult);
        runner.add("window owner: kind and position markup", testWindowKindMarkup);
    }
}
