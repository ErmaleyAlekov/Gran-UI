#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "common/markdown.hpp"
#include <string>

namespace
{
    void testParseInlineSimple()
    {
        auto result = chat::parseInline(u8"a **b** c");
        TEST_ASSERT(result.size() == 3);
        TEST_ASSERT(result[0].text == "a ");
        TEST_ASSERT(result[0].style == chat::INLINE_PLAIN);
        TEST_ASSERT(result[1].text == "b");
        TEST_ASSERT(result[1].style == chat::INLINE_BOLD);
        TEST_ASSERT(result[2].text == " c");
        TEST_ASSERT(result[2].style == chat::INLINE_PLAIN);
    }

    void testParseInlineCode()
    {
        auto result = chat::parseInline(u8"use `x*y` now");
        TEST_ASSERT(result.size() == 3);
        TEST_ASSERT(result[0].text == "use ");
        TEST_ASSERT(result[0].style == chat::INLINE_PLAIN);
        TEST_ASSERT(result[1].text == "x*y");
        TEST_ASSERT(result[1].style == chat::INLINE_CODE);
        TEST_ASSERT(result[2].text == " now");
        TEST_ASSERT(result[2].style == chat::INLINE_PLAIN);
    }

    void testParseInlineMixed()
    {
        auto result = chat::parseInline(u8"*it* and **bo**");
        TEST_ASSERT(result.size() == 3);
        TEST_ASSERT(result[0].text == "it");
        TEST_ASSERT(result[0].style == chat::INLINE_ITALIC);
        TEST_ASSERT(result[1].text == " and ");
        TEST_ASSERT(result[1].style == chat::INLINE_PLAIN);
        TEST_ASSERT(result[2].text == "bo");
        TEST_ASSERT(result[2].style == chat::INLINE_BOLD);
    }

    void testParseInlineUnclosedMarkers()
    {
        auto result = chat::parseInline(u8"2 * 3 = 6");
        TEST_ASSERT(result.size() == 1);
        TEST_ASSERT(result[0].text == "2 * 3 = 6");
        TEST_ASSERT(result[0].style == chat::INLINE_PLAIN);

        result = chat::parseInline(u8"**open");
        TEST_ASSERT(result.size() == 1);
        TEST_ASSERT(result[0].text == "**open");
        TEST_ASSERT(result[0].style == chat::INLINE_PLAIN);

        result = chat::parseInline(u8"a `b");
        TEST_ASSERT(result.size() == 1);
        TEST_ASSERT(result[0].text == "a `b");
        TEST_ASSERT(result[0].style == chat::INLINE_PLAIN);
    }

    void testParseInlineMultipleStyles()
    {
        auto result = chat::parseInline(u8"***x***");
        TEST_ASSERT(result.size() == 1);
        TEST_ASSERT(result[0].text == "x");
        TEST_ASSERT(result[0].style == (chat::INLINE_BOLD | chat::INLINE_ITALIC));
    }

    void testParseMarkdownParagraphs()
    {
        auto result = chat::parseMarkdown(u8"para one\nline two\n\npara two");
        TEST_ASSERT(result.size() == 2);
        TEST_ASSERT(result[0].kind == chat::BlockKind::PARAGRAPH);
        TEST_ASSERT(result[0].runs.size() == 1);
        TEST_ASSERT(result[0].runs[0].text == "para one\nline two");
        TEST_ASSERT(result[1].kind == chat::BlockKind::PARAGRAPH);
        TEST_ASSERT(result[1].runs.size() == 1);
        TEST_ASSERT(result[1].runs[0].text == "para two");
    }

    void testParseMarkdownHeading()
    {
        auto result = chat::parseMarkdown(u8"# Title\ntext");
        TEST_ASSERT(result.size() == 2);
        TEST_ASSERT(result[0].kind == chat::BlockKind::HEADING);
        TEST_ASSERT(result[0].level == 1);
        TEST_ASSERT(result[0].runs.size() == 1);
        TEST_ASSERT(result[0].runs[0].text == "Title");
        TEST_ASSERT(result[1].kind == chat::BlockKind::PARAGRAPH);
        TEST_ASSERT(result[1].runs.size() == 1);
        TEST_ASSERT(result[1].runs[0].text == "text");
    }

    void testParseMarkdownCodeBlock()
    {
        auto result = chat::parseMarkdown(u8"before\n```cpp\nint a;\n\nint b;\n```\nafter");
        TEST_ASSERT(result.size() == 3);
        TEST_ASSERT(result[0].kind == chat::BlockKind::PARAGRAPH);
        TEST_ASSERT(result[0].runs.size() == 1);
        TEST_ASSERT(result[0].runs[0].text == "before");
        TEST_ASSERT(result[1].kind == chat::BlockKind::CODE);
        TEST_ASSERT(result[1].language == "cpp");
        TEST_ASSERT(result[1].runs.size() == 1);
        TEST_ASSERT(result[1].runs[0].text == "int a;\n\nint b;");
        TEST_ASSERT(result[2].kind == chat::BlockKind::PARAGRAPH);
        TEST_ASSERT(result[2].runs.size() == 1);
        TEST_ASSERT(result[2].runs[0].text == "after");
    }

    void testParseMarkdownUnclosedCodeBlock()
    {
        auto result = chat::parseMarkdown(u8"```\ncode");
        TEST_ASSERT(result.size() == 1);
        TEST_ASSERT(result[0].kind == chat::BlockKind::CODE);
        TEST_ASSERT(result[0].language.empty());
        TEST_ASSERT(result[0].runs.size() == 1);
        TEST_ASSERT(result[0].runs[0].text == "code");
    }

    void testParseMarkdownList()
    {
        auto result = chat::parseMarkdown(u8"- one\n* two");
        TEST_ASSERT(result.size() == 1);
        TEST_ASSERT(result[0].kind == chat::BlockKind::PARAGRAPH);
        TEST_ASSERT(result[0].runs.size() == 1);
        TEST_ASSERT(result[0].runs[0].text == u8"• one\n• two");
    }

    void testParseMarkdownEmpty()
    {
        auto result = chat::parseMarkdown(u8"");
        TEST_ASSERT(result.empty());
    }
}

namespace suites
{
    void registerChatMarkdownTests(test::TestRunner& runner)
    {
        runner.add("chat: markdown simple inline", testParseInlineSimple);
        runner.add("chat: markdown code inline", testParseInlineCode);
        runner.add("chat: markdown mixed inline", testParseInlineMixed);
        runner.add("chat: markdown unclosed markers inline", testParseInlineUnclosedMarkers);
        runner.add("chat: markdown multiple styles inline", testParseInlineMultipleStyles);
        runner.add("chat: markdown paragraphs", testParseMarkdownParagraphs);
        runner.add("chat: markdown heading", testParseMarkdownHeading);
        runner.add("chat: markdown code block", testParseMarkdownCodeBlock);
        runner.add("chat: markdown unclosed code block", testParseMarkdownUnclosedCodeBlock);
        runner.add("chat: markdown list", testParseMarkdownList);
        runner.add("chat: markdown empty", testParseMarkdownEmpty);
    }
}
