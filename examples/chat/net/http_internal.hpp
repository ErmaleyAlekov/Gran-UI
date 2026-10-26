#ifndef CHAT_NET_HTTP_INTERNAL_HPP
#define CHAT_NET_HTTP_INTERNAL_HPP

#include "common/chunked_decoder.hpp"
#include "net/http_client.hpp"
#include "net/tcp_connection.hpp"

#include <string>

namespace chat::http
{
    // Итог чтения из сети с учётом отмены / The outcome of a network read considering cancel
    enum class ReadStatus : std::uint8_t { DATA, CLOSED, CANCELLED };

    // ---- http_client.cpp ----

    // Ждать данных, опрашивая отмену; DATA — байты дописаны в out / Wait for data while polling cancel; DATA — the bytes are appended to out
    ReadStatus readMore(TcpConnection& connection, HttpReceiver& receiver, std::string& out);

    // ---- http_body.cpp ----

    // Граница тела ответа: chunked, Content-Length или закрытие соединения / The response body boundary: chunked, Content-Length or the connection close
    struct BodyFraming
    {
        const HttpHead& head;
        ChunkedDecoder decoder;
        long long remaining = -1;

        // Сырые байты → полезные данные получателю / Raw bytes → the payload for the receiver
        void feed(const std::string& raw, HttpReceiver& receiver);

        // Тело дочитано (для «до закрытия» — никогда) / The body is fully read (for "until close" — never)
        bool finished() const noexcept;
    };

    // Тело после заголовка; pending — байты тела, пришедшие вместе с заголовком / The body after the head; pending — the body bytes that arrived with the head
    bool readBody(TcpConnection& connection, const HttpHead& head, std::string pending, HttpReceiver& receiver);
}

#endif
