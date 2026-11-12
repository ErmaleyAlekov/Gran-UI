#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"
#include "gran/layout/layout.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"
#include <cmath>

namespace
{
    const char* MARKUP = R"(
        <Window>
            <Stack grow="1">
                <DockHost id="host" grow="1">
                    <Text id="center">Центр</Text>
                    <DockPanel id="right" title="Справа" dock="right" size="150">
                        <Text>1</Text>
                    </DockPanel>
                    <DockPanel id="out" title="Вывод" dock="bottom" size="100">
                        <Text>2</Text>
                    </DockPanel>
                    <DockPanel id="err" title="Ошибки" dock="bottom">
                        <Text>3</Text>
                    </DockPanel>
                </DockHost>
            </Stack>
        </Window>
    )";

    void testDockTitleAfterMove()
    {
        auto doc = support::layoutMarkup(MARKUP, 800, 600);
        auto* host = gran::ui::findNode(*doc.root, "host");
        auto* right = gran::ui::findNode(*doc.root, "right");

        support::FixedMeasurer m;
        gran::layout::layoutTree(*doc.root, gran::Size{800, 600}, m);

        right->content.dock.side = gran::ui::DockSide::BOTTOM;
        gran::ui::chooseDockPanel(*right);
        gran::layout::layoutTree(*doc.root, gran::Size{800, 600}, m);

        auto* strip = gran::ui::dockStrip(*host, gran::ui::DockSide::BOTTOM);
        TEST_ASSERT(strip->children.size() == 3);
        TEST_ASSERT(strip->children[0]->visible);
        TEST_ASSERT(strip->children[0]->content.checked);
        TEST_ASSERT(strip->children[0]->layout.frame.width > 0);
        TEST_ASSERT(strip->children[1]->layout.frame.x >= strip->children[0]->layout.frame.x + strip->children[0]->layout.frame.width - 0.5f);
        TEST_ASSERT(right->visible);
        TEST_ASSERT(!gran::ui::findNode(*doc.root, "out")->visible);
    }

    const char* MARKUP_TABS = R"(
        <Window>
            <Stack grow="1">
                <Tabs id="docs" reorderable="true">
                    <Tab id="a" text="Один"/>
                    <Tab id="b" text="Два"/>
                    <Tab id="c" text="Три"/>
                </Tabs>
            </Stack>
        </Window>
    )";

    void testTabHeadersAfterMove()
    {
        auto doc = support::layoutMarkup(MARKUP_TABS, 600, 400);
        auto* docs = gran::ui::findNode(*doc.root, "docs");
        auto* bar = docs->children[0].get();

        TEST_ASSERT(bar->children.size() == 3);
        support::FixedMeasurer m;
        gran::layout::layoutTree(*doc.root, gran::Size{600, 400}, m);
        float xB = bar->children[1]->layout.frame.x;

        TEST_ASSERT(gran::ui::moveTab(*docs, 0, 2));
        gran::layout::layoutTree(*doc.root, gran::Size{600, 400}, m);

        TEST_ASSERT(bar->children[2]->content.text == u8"Один");
        TEST_ASSERT(bar->children[0]->layout.frame.x < bar->children[2]->layout.frame.x);
        TEST_ASSERT(bar->children[0]->layout.frame.x < xB);
    }
}

namespace suites
{
    void registerLayoutCacheTests(test::TestRunner& runner)
    {
        runner.add("layout cache: dock title after move", testDockTitleAfterMove);
        runner.add("layout cache: tab headers after move", testTabHeadersAfterMove);
    }
}
