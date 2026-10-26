#include "common/chat_texts.hpp"

namespace chat
{
    namespace
    {
        // Английские тексты; {N} — аргументы say() / English texts; {N} are the say() arguments
        const ChatTextEntry TEXTS[] = {
            {ChatText::STATUS_CONNECTING, "connecting to Ollama…"},
            {ChatText::STATUS_READY, "ready"},
            {ChatText::STATUS_THINKING, "thinking…"},
            {ChatText::STATUS_ANSWERING, "answering…"},
            {ChatText::STATUS_STOPPED, "the answer was stopped"},
            {ChatText::STATUS_OFFLINE, "Ollama does not answer at {0} — run \"ollama serve\""},
            {ChatText::STATUS_MODEL_MISSING, "the model {0} is not installed — \"ollama pull {0}\""},
            {ChatText::STATUS_FAILED, "error: {0}"},
            {ChatText::JSON_UNEXPECTED_END, "the JSON is cut off (position {0})"},
            {ChatText::JSON_UNEXPECTED_CHAR, "an unexpected character \"{0}\" in the JSON (position {1})"},
            {ChatText::JSON_BAD_ESCAPE, "an invalid escape sequence in a JSON string (position {0})"},
            {ChatText::JSON_BAD_NUMBER, "an invalid number in the JSON (position {0})"},
            {ChatText::JSON_TOO_DEEP, "the JSON is nested deeper than {0} levels"},
            {ChatText::CHUNK_BAD_SIZE, "an invalid chunked block size: \"{0}\""},
            {ChatText::CHUNK_BAD_END, "a chunked block does not end with a line break"},
            {ChatText::LINE_TOO_LONG, "a response line is longer than {0} bytes"},
            {ChatText::HTTP_BAD_HEAD, "an invalid HTTP response head"},
            {ChatText::HTTP_STATUS, "the server answered {0} {1}"},
            {ChatText::NET_STARTUP, "failed to start WinSock (code {0})"},
            {ChatText::NET_CONNECT, "failed to connect to {0}:{1}"},
            {ChatText::NET_SEND, "failed to send the request (code {0})"},
            {ChatText::NET_RECEIVE, "failed to receive the response (code {0})"},
            {ChatText::NET_CLOSED, "the server closed the connection before the end of the response"},
            {ChatText::OLLAMA_ERROR, "Ollama: {0}"},
            {ChatText::APP_FATAL, "Gran Chat: {0}"},
            {ChatText::APP_MISSING_NODE, "the chat markup has no element id=\"{0}\""}
        };
    }

    // Каталог английского языка / The English language catalog
    const ChatCatalog CHAT_ENGLISH = {"en", TEXTS, sizeof(TEXTS) / sizeof(TEXTS[0])};
}
