#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"
#include "gran/layout/layout.hpp"
#include "gran/markup/markup_loader.hpp"
#include <string>

namespace
{
    // Повторная компоновка того же документа в другом окне / A repeated layout of the same document in another window
    void layout(gran::markup::UiDocument& doc, gran::Size viewport)
    {
        support::FixedMeasurer measurer;

        gran::ui::markDirty(*doc.root, gran::ui::DIRTY_LAYOUT);
        gran::layout::layoutTree(*doc.root, viewport, measurer);
    }

    // Низ рамки узла / The node frame bottom
    float bottom(const gran::ui::Node& node)
    {
        return node.layout.frame.y + node.layout.frame.height;
    }

    void testFlexAutoMinTextAboveScroll()
    {
        auto doc = support::layoutMarkup(R"(<Window><Stack><Text id="t">Заголовок</Text><Scroll id="s" grow="1"><Stack height="1000"/></Scroll></Stack></Window>)", 400.0f, 2000.0f);
        layout(doc, gran::Size{400.0f, 2000.0f});
        auto* t = gran::ui::findNode(*doc.root, "t");
        auto* s = gran::ui::findNode(*doc.root, "s");
        TEST_ASSERT(t != nullptr && s != nullptr);
        float h = t->layout.frame.height;

        layout(doc, gran::Size{400.0f, 200.0f});
        TEST_ASSERT_NEAR(t->layout.frame.height, h, 0.5f);
        TEST_ASSERT_NEAR(s->layout.frame.y, t->layout.frame.y + h, 0.5f);
        TEST_ASSERT_NEAR(s->layout.frame.height, 200.0f - h, 0.5f);
    }

    void testFlexAutoMinNestedStack()
    {
        auto doc = support::layoutMarkup(R"(<Window><Stack><Stack id="inner" grow="1"><Text id="t">Заголовок</Text><Scroll grow="1"><Stack height="1000"/></Scroll></Stack><Text id="foot">Подвал</Text></Stack></Window>)", 400.0f, 200.0f);
        layout(doc, gran::Size{400.0f, 200.0f});
        auto* inner = gran::ui::findNode(*doc.root, "inner");
        auto* t = gran::ui::findNode(*doc.root, "t");
        auto* foot = gran::ui::findNode(*doc.root, "foot");
        TEST_ASSERT(inner != nullptr && t != nullptr && foot != nullptr);
        float h = t->layout.frame.height;
        TEST_ASSERT(bottom(*foot) <= 200.5f);
        TEST_ASSERT(foot->layout.frame.height > 0.0f);
        TEST_ASSERT(bottom(*inner) <= foot->layout.frame.y + 0.5f);
        layout(doc, gran::Size{400.0f, 2000.0f});
        TEST_ASSERT_NEAR(t->layout.frame.height, h, 0.5f);
    }

    void testFlexAutoMinOverflowHiddenShrinks()
    {
        auto doc = support::layoutMarkup(R"(<Window><Stack><Box id="b" overflow="hidden"><Box height="500"/></Box><Text id="t">Подпись</Text></Stack></Window>)", 400.0f, 200.0f);
        layout(doc, gran::Size{400.0f, 200.0f});
        auto* b = gran::ui::findNode(*doc.root, "b");
        auto* t = gran::ui::findNode(*doc.root, "t");
        TEST_ASSERT(b != nullptr && t != nullptr);
        TEST_ASSERT(b->layout.frame.height < 500.0f);
        TEST_ASSERT(t->layout.frame.height > 0.0f);
        TEST_ASSERT(bottom(*t) <= 200.5f);
    }

    void testFlexAutoMinHorizontalStillShrinks()
    {
        auto doc = support::layoutMarkup(R"(<Window><Stack orientation="horizontal" width="100"><Text id="t">очень длинная строка текста без переносов для проверки сжатия по горизонтали</Text></Stack></Window>)", 400.0f, 200.0f);
        layout(doc, gran::Size{400.0f, 200.0f});
        auto* t = gran::ui::findNode(*doc.root, "t");
        TEST_ASSERT(t != nullptr);
        TEST_ASSERT(t->layout.frame.width <= 100.5f);
    }
}

namespace suites
{
    void registerFlexAutoMinTests(test::TestRunner& runner)
    {
        runner.add("flex auto min: text above scroll", testFlexAutoMinTextAboveScroll);
        runner.add("flex auto min: nested stack", testFlexAutoMinNestedStack);
        runner.add("flex auto min: overflow hidden shrinks", testFlexAutoMinOverflowHiddenShrinks);
        runner.add("flex auto min: horizontal still shrinks", testFlexAutoMinHorizontalStillShrinks);
    }
}
