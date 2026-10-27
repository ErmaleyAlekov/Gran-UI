#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"
#include "gran/binding/list_bindings.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/layout/layout.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/markup/theme.hpp"
#include "gran/ui/style_pass.hpp"
#include <string>
#include <vector>

namespace
{
    void settle(gran::markup::UiDocument& doc, gran::binding::ListBindings& lists, support::FixedMeasurer& measurer)
    {
        for (int i = 0; i < 5; ++i)
        {
            gran::ui::resolveStyles(*doc.root, gran::markup::windowStyle(gran::markup::defaultTheme(), doc.window), gran::markup::defaultTheme().kinds);
            gran::layout::layoutTree(*doc.root, gran::Size{400, 300}, measurer);
            if (!lists.updateVirtual())
                break;
        }
    }

    void testIncrementalLayout()
    {
        support::FixedMeasurer measurer;
        auto doc = gran::markup::loadMarkupString(R"(<Window><Scroll id="s"><Stack id="list" items="rows" spacing="4"><ItemTemplate><Text>{t}</Text></ItemTemplate></Stack></Scroll></Window>)", gran::markup::LoadOptions{});
        gran::binding::ListBindings lists;
        for (std::size_t i = 0; i < 50; ++i)
        {
            lists.model("rows").append({{"t", "строка " + std::to_string(i)}});
        }
        lists.bind(*doc.root);

        settle(doc, lists, measurer);

        auto* list = gran::ui::findNode(*doc.root, "list");
        TEST_ASSERT(list != nullptr);

        // Локальная перекомпоновка после правки записи 10 / A local relayout after editing record 10
        lists.model("rows").set(10, "t", std::string(200, 'x'));
        settle(doc, lists, measurer);
        auto originalFrames = std::vector<gran::Rect>{};
        for (const auto& child : list->children)
            originalFrames.push_back(child->layout.frame);
        TEST_ASSERT(originalFrames[10].height > originalFrames[9].height);

        // Полная перекомпоновка того же дерева должна дать те же рамки / A full relayout of the same tree must give the same frames
        for (const auto& child : list->children)
            gran::ui::markDirty(*child, gran::ui::DIRTY_LAYOUT);
        gran::ui::markDirty(*doc.root, gran::ui::DIRTY_LAYOUT);
        settle(doc, lists, measurer);

        for (std::size_t i = 0; i < list->children.size(); ++i)
        {
            TEST_ASSERT_NEAR(list->children[i]->layout.frame.y, originalFrames[i].y, 0.001f);
            TEST_ASSERT_NEAR(list->children[i]->layout.frame.height, originalFrames[i].height, 0.001f);
        }
    }

    void testVirtualList()
    {
        support::FixedMeasurer measurer;
        auto doc = gran::markup::loadMarkupString(R"(<Window><Scroll id="s"><Stack id="list" items="rows" spacing="4" virtual="true"><ItemTemplate><Text>{t}</Text></ItemTemplate></Stack></Scroll></Window>)", gran::markup::LoadOptions{});
        gran::binding::ListBindings lists;
        for (std::size_t i = 0; i < 10000; ++i)
        {
            lists.model("rows").append({{"t", "строка " + std::to_string(i)}});
        }
        lists.bind(*doc.root);

        settle(doc, lists, measurer);

        auto* list = gran::ui::findNode(*doc.root, "list");
        TEST_ASSERT(list != nullptr);
        TEST_ASSERT(list->children.size() < 100);
        TEST_ASSERT(list->layout.frame.height > 10000 * 10);

        auto* s = gran::ui::findNode(*doc.root, "s");
        TEST_ASSERT(s != nullptr);
        s->layout.scrollOffset.y = 50000;
        gran::ui::markDirty(*s, gran::ui::DIRTY_LAYOUT);
        settle(doc, lists, measurer);

        for (const auto& child : list->children)
        {
            if (!child->content.text.empty())
            {
                auto itemRef = lists.itemOf(*child);
                TEST_ASSERT(itemRef.model != nullptr);
                TEST_ASSERT(itemRef.index > 1000);
                break;
            }
        }

        lists.model("rows").set(5, "t", "z");
        settle(doc, lists, measurer);
        TEST_ASSERT(list->children.size() < 100);

        lists.model("rows").remove(0);
        TEST_ASSERT(lists.model("rows").size() == 9999);
    }

    void testVirtualListWithoutItems()
    {
        TEST_ASSERT_THROWS(gran::markup::loadMarkupString(R"(<Window><Stack virtual="true"/></Window>)", gran::markup::LoadOptions{}), gran::MarkupException);
    }
}

namespace suites
{
    void registerIncrementalLayoutTests(test::TestRunner& runner)
    {
        runner.add("incremental layout: basic functionality", testIncrementalLayout);
        runner.add("virtual list: basic functionality", testVirtualList);
        runner.add("virtual list: without items", testVirtualListWithoutItems);
    }
}
