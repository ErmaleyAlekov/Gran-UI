#ifndef CHAT_NET_HTTP_CLIENT_HPP
#define CHAT_NET_HTTP_CLIENT_HPP

#include "common/http_head.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace chat
{
    // Запрос HTTP/1.1 (соединение на один запрос: Connection: close) / An HTTP/1.1 request (a connection per request: Connection: close)
    struct HttpRequest
    {
        std::string method = "GET";
        std::string host = "127.0.0.1";
        std::uint16_t port = 80;
        std::string path = "/";
        std::string contentType;
        std::string body;
    };

    /*
    ** Получатель ответа (паттерн Observer): сначала заголовок, затем куски тела по мере
    ** прихода — уже без chunked-кодирования. cancelled() опрашивается между ожиданиями сети
    ** (не реже раза в CANCEL_POLL_MS): true — прервать чтение.
    **
    ** The response receiver (the Observer pattern): the head first, then the body pieces as they
    ** arrive — already without the chunked encoding. cancelled() is polled between network waits
    ** (at least once per CANCEL_POLL_MS): true — stop reading.
    */
    class HttpReceiver
    {
    public:
        virtual ~HttpReceiver() = default;

        // Заголовок ответа пришёл / The response head has arrived
        virtual void onHead(const HttpHead& head) = 0;

        // Очередной кусок тела / The next body piece
        virtual void onBody(std::string_view data) = 0;

        // Прервать ли чтение / Whether to stop reading
        virtual bool cancelled() const noexcept = 0;
    };

    // Период опроса отмены, мс / The cancel polling period, ms
    constexpr int CANCEL_POLL_MS = 100;

    // ---- http_client.cpp ----

    /*
    ** Отправить запрос и прочитать ответ целиком (тело — потоком в receiver). Длина тела —
    ** по chunked, Content-Length или до закрытия соединения. Возврат false — прервано отменой;
    ** сбои — NetworkException / ProtocolException.
    **
    ** Send a request and read the whole response (the body streams into the receiver). The body length —
    ** by chunked, Content-Length or until the connection closes. A false return — stopped by cancel;
    ** failures — NetworkException / ProtocolException.
    */
    bool performRequest(const HttpRequest& request, HttpReceiver& receiver);
}

#endif
