#ifndef CHAT_OLLAMA_OLLAMA_CLIENT_HPP
#define CHAT_OLLAMA_OLLAMA_CLIENT_HPP

#include "common/ollama_protocol.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace chat
{
    // Адрес сервера Ollama и модель / The Ollama server address and the model
    struct OllamaEndpoint
    {
        std::string host = "127.0.0.1";
        std::uint16_t port = 11434;
        std::string model = "qwen2.5-coder:14b";
    };

    /*
    ** Слушатель потокового ответа: куски текста по мере генерации. Вызывается в сетевом
    ** потоке — сам передаёт данные в поток интерфейса (Application::post).
    **
    ** The streaming reply listener: text pieces as they are generated. It is called in the network
    ** thread — it passes the data to the UI thread itself (Application::post).
    */
    class ReplyListener
    {
    public:
        virtual ~ReplyListener() = default;

        // Очередной кусок ответа модели / The next piece of the model reply
        virtual void onPiece(const std::string& text) = 0;

        // Прервать ли ответ (кнопка «Остановить», закрытие окна) / Whether to stop the reply (the "Stop" button, closing the window)
        virtual bool cancelled() const noexcept = 0;
    };

    // ---- ollama_client.cpp ----

    /*
    ** POST /api/chat со stream:true: история целиком, ответ — строками NDJSON. Возврат
    ** false — прервано отменой. Ошибка Ollama ({"error":…}) — ChatException(OLLAMA_ERROR),
    ** прочий ответ не 200 — HttpException, сеть — NetworkException.
    **
    ** POST /api/chat with stream:true: the whole history, the reply comes as NDJSON lines. A false
    ** return — stopped by cancel. An Ollama error ({"error":…}) — ChatException(OLLAMA_ERROR),
    ** another non-200 reply — HttpException, the network — NetworkException.
    */
    bool streamChat(const OllamaEndpoint& endpoint, const std::vector<ChatMessage>& history, ReplyListener& listener);

    // ---- ollama_probe.cpp ----

    // GET /api/tags: установлена ли модель; сервер недоступен — NetworkException / GET /api/tags: whether the model is installed; the server is unreachable — NetworkException
    bool probeModel(const OllamaEndpoint& endpoint);
}

#endif
