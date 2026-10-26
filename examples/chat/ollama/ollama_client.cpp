#include "ollama/ollama_client.hpp"
#include "common/chat_errors.hpp"
#include "common/json_value.hpp"
#include "ollama/chat_receiver.hpp"

#include <string>

namespace chat
{
    namespace
    {
        // Ответ не 200: текст ошибки Ollama из JSON, если он есть, иначе код и причина / A non-200 reply: the Ollama error text from JSON if present, otherwise the code and reason
        [[noreturn]] void failOnStatus(const ChatReceiver& receiver)
        {
            std::string error;

            try
            {
                error = jsonString(parseJson(receiver.failure), "error");
            }
            catch (const JsonException&)
            {
                // Тело не JSON (прокси, другой сервер) — остаётся код ответа / The body is not JSON (a proxy, another server) — the status code remains
            }
            if (!error.empty())
                throw ChatException(ChatText::OLLAMA_ERROR, {error});
            throw HttpException(receiver.head.status, {std::to_string(receiver.head.status), receiver.head.reason});
        }
    }

    // Хвост без LF разбирается так же, как полная строка / A tail without LF is parsed as a full line
    void ChatReceiver::finish()
    {
        std::string rest = _splitter.flush();

        if (head.status == 200 && !rest.empty())
            handleLine(rest);
    }

    // Запрос, поток, проверка кода / The request, the stream, the status check
    bool streamChat(const OllamaEndpoint& endpoint, const std::vector<ChatMessage>& history, ReplyListener& listener)
    {
        HttpRequest request;
        ChatReceiver receiver(listener);

        request.method = "POST";
        request.host = endpoint.host;
        request.port = endpoint.port;
        request.path = "/api/chat";
        request.contentType = "application/json";
        request.body = buildChatRequest(endpoint.model, history);
        if (!performRequest(request, receiver))
            return false;
        if (receiver.head.status != 200)
            failOnStatus(receiver);
        receiver.finish();
        return true;
    }
}
