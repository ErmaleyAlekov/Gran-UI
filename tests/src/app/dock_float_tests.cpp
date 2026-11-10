#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/app/commands.hpp"
#include "gran/app/internal/app_state.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"

#include <string>

namespace
{
    // Окно в собственном состоянии теста (без окна ОС) / A window in the test own state (without an OS window)
    gran::app::AppWindow& addTestWindow(gran::Application::State& state, const char* markup)
    {
        return gran::app::addWindow(state, gran::markup::loadMarkupString(markup, gran::markup::LoadOptions{}), ".");
    }

    void testFloatAndReturn()
    {
        gran::Application::State state;
        const char* mainMarkup = "<Window><DockHost id=\"host\"><Text>центр</Text><DockPanel id=\"p\" title=\"Панель\" dock=\"left\"><Text id=\"t\">x</Text><Input id=\"i\"/></DockPanel></DockHost></Window>";
        const char* floatMarkup = "<Window><Stack/></Window>";
        gran::app::AppWindow& main = addTestWindow(state, mainMarkup);
        gran::app::AppWindow& fw = addTestWindow(state, floatMarkup);
        gran::ui::Node* p = gran::ui::findNode(*main.document.root, "p");
        gran::ui::Node* input = gran::ui::findNode(*main.document.root, "i");

        TEST_ASSERT(p != nullptr);
        TEST_ASSERT(input != nullptr);

        gran::app::floatPanel(state, *p, fw);

        TEST_ASSERT(p->children.empty());
        TEST_ASSERT(p->content.dock.floating);
        TEST_ASSERT(fw.document.root->children.size() == 2);
        TEST_ASSERT(gran::app::floatWindowOf(state, *p) == &fw);

        input->content.text = "edited";

        gran::app::returnPanel(state, fw);

        TEST_ASSERT(p->children.size() == 2);
        TEST_ASSERT(!p->content.dock.floating);
        TEST_ASSERT(p->children[1].get() == input);
        TEST_ASSERT(input->content.text == "edited");
        TEST_ASSERT(gran::app::floatWindowOf(state, *p) == nullptr);
        TEST_ASSERT(fw.floatingPanel == nullptr);
    }

    void testReturnWithoutPanel()
    {
        gran::Application::State state;
        const char* floatMarkup = "<Window><Stack/></Window>";
        // Окно панели не бывает первым: первое — главное окно программы / A panel window is never the first one: the first is the main program window
        addTestWindow(state, "<Window><Text>главное</Text></Window>");
        gran::app::AppWindow& fw = addTestWindow(state, floatMarkup);

        TEST_ASSERT(fw.document.root->children.empty());
        gran::app::returnPanel(state, fw);

        TEST_ASSERT(fw.document.root->children.empty());
    }
}

namespace suites
{
    void registerDockFloatTests(test::TestRunner& runner)
    {
        runner.add("dock float: float and return", testFloatAndReturn);
        runner.add("dock float: return without panel", testReturnWithoutPanel);
    }
}
