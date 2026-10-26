#include "ollama/ollama_client.hpp"
#include "common/chat_errors.hpp"
#include "net/http_client.hpp"

#include <string>

namespace chat
{
    namespace
    {
        // Приёмник короткого ответа целиком (список моделей — единицы килобайт) / A receiver of a short whole reply (the model list is a few kilobytes)
        class TextReceiver final : public HttpReceiver
        {
        public:
            // Заголовок запоминается / The head is remembered
            void onHead(const HttpHead& received) override
            {
                head = received;
            }

            // Тело копится / The body is accumulated
            void onBody(std::string_view data) override
            {
                body.append(data);
            }

            // Проверка не отменяется: она короткая / The probe is not cancelled: it is short
            bool cancelled() const noexcept override
            {
                return false;
            }

            HttpHead head;
            std::string body;
        };
    }

    // Не 200 — ошибка HTTP; 200 — поиск модели в списке / Not 200 — an HTTP error; 200 — searching the model in the list
    bool probeModel(const OllamaEndpoint& endpoint)
    {
        HttpRequest request;
        TextReceiver receiver;

        request.host = endpoint.host;
        request.port = endpoint.port;
        request.path = "/api/tags";
        performRequest(request, receiver);
        if (receiver.head.status != 200)
            throw HttpException(receiver.head.status, {std::to_string(receiver.head.status), receiver.head.reason});
        return modelInstalled(receiver.body, endpoint.model);
    }
}
