#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "common/chat_errors.hpp"
#include "common/http_head.hpp"
#include "common/json_value.hpp"
#include "common/markdown.hpp"
#include "platform/chat_options.hpp"
#include "ui/chat_view.hpp"

namespace
{
    void checkJsonError(std::string_view input, chat::ChatText expected)
    {
        try
        {
            chat::parseJson(input);
        }
        catch (const chat::JsonException& error)
        {
            TEST_ASSERT(error.getId() == expected);
            return;
        }
        TEST_ASSERT(false);
    }

    void testJsonDiagnostics()
    {
        checkJsonError("[01]", chat::ChatText::JSON_BAD_NUMBER);
        checkJsonError("1e500", chat::ChatText::JSON_BAD_NUMBER);
        checkJsonError("\"a\nb\"", chat::ChatText::JSON_CONTROL_CHAR);
    }

    void testMarkdownDelimiters()
    {
        const auto inlineRuns = chat::parseInline("*a * b");
        TEST_ASSERT(inlineRuns.size() == 1);
        TEST_ASSERT(inlineRuns[0].style == chat::INLINE_PLAIN);
        TEST_ASSERT(inlineRuns[0].text == "*a * b");

        const auto italicRuns = chat::parseInline("**word*");
        TEST_ASSERT(italicRuns.size() == 2);
        TEST_ASSERT(italicRuns[0].style == chat::INLINE_PLAIN && italicRuns[0].text == "*");
        TEST_ASSERT(italicRuns[1].style == chat::INLINE_ITALIC && italicRuns[1].text == "word");

        const auto literalPair = chat::parseInline("*a ** b*");
        TEST_ASSERT(literalPair.size() == 1);
        TEST_ASSERT(literalPair[0].style == chat::INLINE_ITALIC && literalPair[0].text == "a ** b");

        const auto codeRuns = chat::parseInline("*before `x*y`");
        TEST_ASSERT(codeRuns.size() == 2);
        TEST_ASSERT(codeRuns[0].style == chat::INLINE_PLAIN && codeRuns[0].text == "*before ");
        TEST_ASSERT(codeRuns[1].style == chat::INLINE_CODE && codeRuns[1].text == "x*y");

        const auto blocks = chat::parseMarkdown("```cpp\na\n```cpp\nb\n```\nafter");
        TEST_ASSERT(blocks.size() == 2);
        TEST_ASSERT(blocks[0].kind == chat::BlockKind::CODE);
        TEST_ASSERT(blocks[0].runs.size() == 1);
        TEST_ASSERT(blocks[0].runs[0].text == "a\n```cpp\nb");
        TEST_ASSERT(blocks[1].kind == chat::BlockKind::PARAGRAPH);
    }

    void testHttpLengthAndEncoding()
    {
        TEST_ASSERT(chat::parseHttpHead("HTTP/1.1 200 OK\r\nTransfer-Encoding: ChUnKeD").chunked);
        TEST_ASSERT(chat::parseHttpHead("HTTP/1.1 200 OK\r\nContent-Length: 5").contentLength == 5);
        TEST_ASSERT(chat::parseHttpHead("HTTP/1.1 200 OK").contentLength == -1);
        TEST_ASSERT(chat::parseHttpHead("HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Length: 5").contentLength == 5);
        TEST_ASSERT_THROWS(chat::parseHttpHead("HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip"), chat::ProtocolException);
    }

    void testChatCommandLine()
    {
        TEST_ASSERT(chat::parsePort("1") == 1);
        TEST_ASSERT(chat::parsePort("65535") == 65535);
        TEST_ASSERT_THROWS(chat::parsePort("70000"), chat::ChatException);
        TEST_ASSERT_THROWS(chat::parsePort("80xyz"), chat::ChatException);
        TEST_ASSERT_THROWS(chat::parseOptions({"gran_chat", "--model"}), chat::ChatException);
        const auto options = chat::parseOptions({"gran_chat", "--port", "9000", "test.xml"});
        TEST_ASSERT(options.endpoint.port == 9000);
        TEST_ASSERT(options.markup == "test.xml");
    }

    void testChatTemplateValidation()
    {
        gran::AppOptions options;
        options.headless = true;
        gran::Application app(options);
        app.loadString(R"(<Window><Stack><Stack id="feed"/><Stack id="feedScroll"/>
            <Text id="welcome"/><Stack id="userTemplate"/><Stack id="modelTemplate"><Stack id="modelBody"/></Stack>
            <Input id="prompt"/><Button id="send" text="Send"/><Button id="stop" text="Stop"/><Text id="status"/>
            <Stack id="statusDot"/><Text id="modelName"/></Stack></Window>)");
        try
        {
            chat::ChatView view(app, "model");
        }
        catch (const chat::ChatException& error)
        {
            TEST_ASSERT(error.getId() == chat::ChatText::APP_MISSING_NODE);
            TEST_ASSERT(std::string_view(error.what()).find("userBody") != std::string_view::npos);
            return;
        }
        TEST_ASSERT(false);
    }
}

namespace suites
{
    void registerChatInputReviewTests(test::TestRunner& runner)
    {
        runner.add("chat: JSON diagnostics", testJsonDiagnostics);
        runner.add("chat: Markdown delimiters", testMarkdownDelimiters);
        runner.add("chat: HTTP length and encoding", testHttpLengthAndEncoding);
        runner.add("chat: command line option validation", testChatCommandLine);
        runner.add("chat: startup validates message templates", testChatTemplateValidation);
    }
}
