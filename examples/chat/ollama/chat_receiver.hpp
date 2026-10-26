#ifndef CHAT_OLLAMA_CHAT_RECEIVER_HPP
#define CHAT_OLLAMA_CHAT_RECEIVER_HPP

#include "common/line_splitter.hpp"
#include "net/http_client.hpp"
#include "ollama/ollama_client.hpp"

#include <string>

namespace chat
{
    /*
    ** Приёмник ответа /api/chat (паттерн Adapter: HttpReceiver → ReplyListener).
    ** При коде 200 каждая строка NDJSON разбирается сразу, иначе тело копится целиком
    ** для текста ошибки.
    **
    ** The /api/chat response receiver (the Adapter pattern: HttpReceiver → ReplyListener).
    ** With code 200 every NDJSON line is parsed at once, otherwise the body is accumulated entirely
    ** for the error text.
    */
    class ChatReceiver final : public HttpReceiver
    {
    public:
        // ---- chat_receiver.cpp ----
        explicit ChatReceiver(ReplyListener& listener);
        void onHead(const HttpHead& received) override;
        void onBody(std::string_view data) override;
        bool cancelled() const noexcept override;

        // ---- ollama_client.cpp ----

        // Конец потока: последняя строка без перевода строки / The stream end: the last line without a line break
        void finish();

        HttpHead head;
        std::string failure;

    private:
        // ---- chat_receiver.cpp ----

        // Одна строка NDJSON → кусок слушателю; {"error":…} — ChatException / One NDJSON line → a piece for the listener; {"error":…} — ChatException
        void handleLine(const std::string& line);

        ReplyListener& _listener;
        LineSplitter _splitter;
    };
}

#endif
