#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"
#include "gran/ui/node.hpp"
#include "gran/render/text_engine.hpp"
#include "gran/style/computed_style.hpp"

namespace
{
    void testRichTextSpans()
    {
        const char* markup = "<Window><Stack><Text id=\"t\">  Начало  <Span id=\"b\" fontWeight=\"700\"> жирный </Span>  конец  </Text><Text id=\"p\">  простой   текст </Text></Stack></Window>";
        auto doc = support::layoutMarkup(markup, 400, 300);
        const auto* t = gran::ui::findNode(*doc.root, "t");
        const auto* p = gran::ui::findNode(*doc.root, "p");

        TEST_ASSERT(t->children.size() == 3);
        TEST_ASSERT(t->children[0]->content.text == u8"Начало ");
        TEST_ASSERT(t->children[1]->content.text == u8"жирный ");
        TEST_ASSERT(t->children[2]->content.text == u8"конец");
        TEST_ASSERT(t->children[1]->id == "b");
        TEST_ASSERT(t->content.text == u8"Начало жирный конец");
        TEST_ASSERT(gran::ui::hasSpans(*t) == true);

        TEST_ASSERT(p->children.empty());
        TEST_ASSERT(p->content.text == u8"простой текст");
        TEST_ASSERT(gran::ui::hasSpans(*p) == false);

        auto runs = gran::render::textRunsOf(*t);
        TEST_ASSERT(runs.size() == 3);
        TEST_ASSERT(runs[0].begin == 0 && runs[0].end == 13);
        TEST_ASSERT(runs[1].begin == 13 && runs[1].end == 26);
        TEST_ASSERT(runs[2].begin == 26 && runs[2].end == 36);
        TEST_ASSERT(runs[1].style.weight == 700);
        TEST_ASSERT(runs[0].style.weight == 400);

        TEST_ASSERT(gran::render::textRunsOf(*p).empty());

        // Приведение к неконстантной ссылке для joinSpans
        // Casting to non-const reference for joinSpans
        gran::ui::Node& t_non_const = const_cast<gran::ui::Node&>(*t);
        t_non_const.children[2]->content.text = u8"финал";
        gran::ui::joinSpans(t_non_const);
        TEST_ASSERT(t_non_const.content.text == u8"Начало жирный финал");
    }

    void testRichTextUnderline()
    {
        const char* markup = "<Window><Text id=\"u\">a <Span id=\"s\" textDecoration=\"underline\">link</Span></Text></Window>";
        auto doc = support::layoutMarkup(markup, 400, 300);
        const auto* s = gran::ui::findNode(*doc.root, "s");
        const auto* u = gran::ui::findNode(*doc.root, "u");

        TEST_ASSERT(s->computed.textDecoration == static_cast<int>(gran::style::TextDecoration::UNDERLINE));
        TEST_ASSERT(u->children[0]->computed.textDecoration == static_cast<int>(gran::style::TextDecoration::NONE));
    }
}

namespace suites
{
    void registerRichTextTests(test::TestRunner& runner)
    {
        runner.add("richtext: spans", testRichTextSpans);
        runner.add("richtext: underline", testRichTextUnderline);
    }
}
