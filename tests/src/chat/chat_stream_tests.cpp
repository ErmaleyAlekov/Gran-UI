#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "common/chunked_decoder.hpp"
#include "common/line_splitter.hpp"
#include "common/ollama_protocol.hpp"
#include "common/chat_errors.hpp"
#include <string>
#include <vector>

namespace
{
    void testChunkedDecoderSingleByte()
    {
        chat::ChunkedDecoder decoder;
        std::string out;
        std::string stream = "4\r\nWiki\r\n6;ext=1\r\npedia \r\nE\r\nin \r\n\r\nchunks.\r\n0\r\nX-Trailer: 1\r\n\r\n";
        for (char c : stream) {
            decoder.feed(std::string_view(&c, 1), out);
        }
        TEST_ASSERT(out == "Wikipedia in \r\n\r\nchunks.");
        TEST_ASSERT(decoder.finished());
    }

    void testChunkedDecoderWholeStream()
    {
        chat::ChunkedDecoder decoder;
        std::string out;
        decoder.feed("4\r\nWiki\r\n6;ext=1\r\npedia \r\nE\r\nin \r\n\r\nchunks.\r\n0\r\nX-Trailer: 1\r\n\r\n", out);
        TEST_ASSERT(out == "Wikipedia in \r\n\r\nchunks.");
        TEST_ASSERT(decoder.finished());
    }

    void testChunkedDecoderNotFinished()
    {
        chat::ChunkedDecoder decoder;
        std::string out;
        decoder.feed("4\r\nWiki\r\n6;ext=1\r\npedia \r\nE\r\nin \r\n\r\nchunks.\r\n0\r\n", out);
        TEST_ASSERT(!decoder.finished());
    }

    void testChunkedDecoderErrors()
    {
        chat::ChunkedDecoder decoder;
        std::string out;
        chat::ChunkedDecoder second;
        TEST_ASSERT_THROWS(decoder.feed("zz\r\n", out), chat::ProtocolException);
        TEST_ASSERT_THROWS(second.feed("3\r\nabcX", out), chat::ProtocolException);
    }

    void testLineSplitterBasic()
    {
        chat::LineSplitter splitter;
        std::vector<std::string> lines;
        splitter.feed("{\"a\":1}\r\n{\"b", lines);
        splitter.feed("\":2}\n\n", lines);
        TEST_ASSERT(lines.size() == 2);
        TEST_ASSERT(lines[0] == "{\"a\":1}");
        TEST_ASSERT(lines[1] == "{\"b\":2}");
        TEST_ASSERT(splitter.flush() == "");
    }

    void testLineSplitterFlush()
    {
        chat::LineSplitter splitter;
        std::vector<std::string> lines;
        splitter.feed("tail", lines);
        TEST_ASSERT(splitter.flush() == "tail");
    }

    void testLineSplitterMaxLine()
    {
        chat::LineSplitter splitter(8);
        std::vector<std::string> lines;
        TEST_ASSERT_THROWS(splitter.feed("0123456789", lines), chat::ProtocolException);
    }

    void testOllamaParseChatLine()
    {
        chat::ChatChunk chunk = chat::parseChatLine(R"({"message":{"role":"assistant","content":"При"},"done":false})");
        TEST_ASSERT(chunk.content == "При");
        TEST_ASSERT(!chunk.done);

        chunk = chat::parseChatLine(R"({"done":true})");
        TEST_ASSERT(chunk.content.empty());
        TEST_ASSERT(chunk.done);

        chunk = chat::parseChatLine(R"({"error":"model not found"})");
        TEST_ASSERT(chunk.error == "model not found");
    }

    void testOllamaBuildChatRequest()
    {
        chat::ChatMessage message = {"user", "hi"};
        std::string request = chat::buildChatRequest("m", {message});
        TEST_ASSERT(request == R"({"model":"m","stream":true,"messages":[{"role":"user","content":"hi"}]})");
    }

    void testOllamaModelInstalled()
    {
        TEST_ASSERT(chat::modelInstalled(R"({"models":[{"name":"qwen:latest"}]})", "qwen"));
        TEST_ASSERT(!chat::modelInstalled(R"({"models":[{"name":"qwen:latest"}]})", "llama"));
    }
}

namespace suites
{
    void registerChatStreamTests(test::TestRunner& runner)
    {
        runner.add("chat: ChunkedDecoder single byte", testChunkedDecoderSingleByte);
        runner.add("chat: ChunkedDecoder whole stream", testChunkedDecoderWholeStream);
        runner.add("chat: ChunkedDecoder not finished", testChunkedDecoderNotFinished);
        runner.add("chat: ChunkedDecoder errors", testChunkedDecoderErrors);
        runner.add("chat: LineSplitter basic", testLineSplitterBasic);
        runner.add("chat: LineSplitter flush", testLineSplitterFlush);
        runner.add("chat: LineSplitter max line", testLineSplitterMaxLine);
        runner.add("chat: Ollama parse chat line", testOllamaParseChatLine);
        runner.add("chat: Ollama build chat request", testOllamaBuildChatRequest);
        runner.add("chat: Ollama model installed", testOllamaModelInstalled);
    }
}
