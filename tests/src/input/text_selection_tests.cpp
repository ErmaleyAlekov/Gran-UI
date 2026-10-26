#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/input/internal/text_cursor.hpp"
#include "gran/ui/node.hpp"
#include <string>

namespace
{
    void testWordBoundsBasic()
    {
        std::size_t from, to;
        gran::input::internal::wordBounds("hello world", 2, from, to);
        TEST_ASSERT(from == 0 && to == 5);

        gran::input::internal::wordBounds("hello world", 5, from, to);
        TEST_ASSERT(from == 5 && to == 6);
    }

    void testWordBoundsUTF8()
    {
        std::size_t from, to;
        gran::input::internal::wordBounds(u8"Привет, мир", 1, from, to);
        TEST_ASSERT(from == 0 && to == 12);
    }

    void testSelectionRange()
    {
        gran::ui::NodeContent content;
        content.text = "hello world";
        content.caret = 2;
        content.anchor = 7;

        std::size_t from, to;
        TEST_ASSERT(gran::input::internal::selectionRange(content, from, to));
        TEST_ASSERT(from == 2 && to == 7);

        content.caret = content.anchor;
        TEST_ASSERT(!gran::input::internal::selectionRange(content, from, to));
    }

    void testPlaceCaret()
    {
        gran::ui::Node node;
        node.content.text = "hello world";

        gran::input::internal::placeCaret(node, 4, false);
        TEST_ASSERT(node.content.caret == 4 && node.content.anchor == 4);

        gran::input::internal::placeCaret(node, 9, true);
        TEST_ASSERT(node.content.caret == 9 && node.content.anchor == 4);
    }

    void testSelectWord()
    {
        gran::ui::Node node;
        node.content.text = "hello world";

        gran::input::internal::selectWord(node, 7);
        TEST_ASSERT(node.content.anchor == 6 && node.content.caret == 11);
    }
}

namespace suites
{
    void registerTextSelectionTests(test::TestRunner& runner)
    {
        runner.add("input: wordBounds basic", testWordBoundsBasic);
        runner.add("input: wordBounds UTF8", testWordBoundsUTF8);
        runner.add("input: selectionRange", testSelectionRange);
        runner.add("input: placeCaret", testPlaceCaret);
        runner.add("input: selectWord", testSelectWord);
    }
}
