#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/app/application.hpp"
#include "gran/ui/node.hpp"
#include "gran/ui/node.hpp"
#include <memory>
#include <string>

namespace
{
    int count(const gran::ui::Node& parent, gran::ui::ElementKind kind)
    {
        int count = 0;
        for (const auto& child : parent.children)
        {
            if (child->kind == kind)
            {
                count++;
            }
        }
        return count;
    }

    void testToolBarRebuild()
    {
        gran::Application app;
        app.loadString(R"(<Window><ToolBar id="bar"><Button id="b" text="Один"/></ToolBar></Window>)");
        auto* bar = app.find("bar");
        TEST_ASSERT(bar != nullptr);
        app.setMarkupAttribute(*bar, "padding", "8");
        TEST_ASSERT(count(*bar, gran::ui::ElementKind::TOOL_MORE) == 1);
    }

    void testStatusAndSplitterRebuild()
    {
        gran::Application app;
        app.loadString(R"(<Window><Stack><StatusBar id="st" sizeGrip="true"><StatusSection><Text>а</Text></StatusSection></StatusBar><Splitter id="sp"><Box/><Box/></Splitter></Stack></Window>)");
        auto* st = app.find("st");
        auto* sp = app.find("sp");
        TEST_ASSERT(st != nullptr);
        TEST_ASSERT(sp != nullptr);
        app.setMarkupAttribute(*st, "padding", "6");
        TEST_ASSERT(count(*st, gran::ui::ElementKind::SIZE_GRIP) == 1);
        app.setMarkupAttribute(*sp, "position", "120");
        TEST_ASSERT(count(*sp, gran::ui::ElementKind::SPLITTER_HANDLE) == 1);
        TEST_ASSERT(sp->children.size() == 3);
    }

    void testDockRebuild()
    {
        gran::Application app;
        app.loadString(R"(<Window><DockHost id="host"><Text>центр</Text><DockPanel id="p" title="П" dock="right"><Text>x</Text></DockPanel></DockHost></Window>)");
        auto* host = app.find("host");
        TEST_ASSERT(host != nullptr);
        app.setMarkupAttribute(*host, "sizes", "-1 200 -1 -1");
        TEST_ASSERT(count(*host, gran::ui::ElementKind::DOCK_STRIP) == 4);
        TEST_ASSERT(count(*host, gran::ui::ElementKind::DOCK_HANDLE) == 4);
        auto* p = app.find("p");
        TEST_ASSERT(p != nullptr);
        app.setMarkupAttribute(*p, "dock", "left");
        TEST_ASSERT(p->content.dock.side == gran::ui::DockSide::LEFT);
    }

    void testTabRemove()
    {
        gran::Application app;
        app.loadString(R"(<Window><Tabs id="t"><Tab id="a" text="А"/><Tab id="b" text="Б"/></Tabs></Window>)");
        auto* t = app.find("t");
        TEST_ASSERT(t != nullptr);
        app.remove(*app.find("a"));
        TEST_ASSERT(t->children[0]->children.size() == 1);
        TEST_ASSERT(t->content.option == u8"Б");
    }
}

namespace suites
{
    void registerFrameRebuildTests(test::TestRunner& runner)
    {
        runner.add("frame rebuild: toolbar", testToolBarRebuild);
        runner.add("frame rebuild: status and splitter", testStatusAndSplitterRebuild);
        runner.add("frame rebuild: dock", testDockRebuild);
        runner.add("frame rebuild: tab remove", testTabRemove);
    }
}
