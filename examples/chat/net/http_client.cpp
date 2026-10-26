#include "net/http_internal.hpp"
#include "common/chat_errors.hpp"

#include <string>
#include <utility>

namespace chat
{
    namespace
    {
        // Предел заголовка ответа: защита от сервера, который шлёт его бесконечно / The response head limit: protection against a server that sends it endlessly
        constexpr std::size_t MAX_HEAD = 64 * 1024;

        // Строка запроса и поля; Connection: close — соединение на один запрос / The request line and fields; Connection: close — a connection per request
        std::string buildRequestText(const HttpRequest& request)
        {
            std::string text = request.method + " " + request.path + " HTTP/1.1\r\n";

            text += "Host: " + request.host + ":" + std::to_string(request.port) + "\r\n";
            text += "User-Agent: GranChat/1.0\r\nAccept: */*\r\nConnection: close\r\n";
            if (!request.contentType.empty())
                text += "Content-Type: " + request.contentType + "\r\n";
            if (!request.body.empty() || request.method == "POST")
                text += "Content-Length: " + std::to_string(request.body.size()) + "\r\n";
            text += "\r\n";
            text += request.body;
            return text;
        }

        // Байты до пустой строки — заголовок; лишнее (начало тела) остаётся в bytes / The bytes up to the empty line are the head; the extra (the body start) stays in bytes
        http::ReadStatus readHead(TcpConnection& connection, HttpReceiver& receiver, std::string& bytes, HttpHead& head)
        {
            std::size_t end = findHeadEnd(bytes);

            while (end == std::string::npos)
            {
                if (bytes.size() > MAX_HEAD)
                    throw ProtocolException(ChatText::HTTP_BAD_HEAD, {});
                http::ReadStatus status = http::readMore(connection, receiver, bytes);
                if (status == http::ReadStatus::CANCELLED)
                    return status;
                if (status == http::ReadStatus::CLOSED)
                    throw NetworkException(ChatText::NET_CLOSED, {});
                end = findHeadEnd(bytes);
            }
            head = parseHttpHead(std::string_view(bytes).substr(0, end));
            bytes.erase(0, end);
            return http::ReadStatus::DATA;
        }
    }

    namespace http
    {
        // Ожидание короткими отрезками: между ними — проверка отмены / Waiting in short slices: the cancel check goes between them
        ReadStatus readMore(TcpConnection& connection, HttpReceiver& receiver, std::string& out)
        {
            char buffer[16 * 1024];
            int received = 0;

            while (!receiver.cancelled())
            {
                ReceiveStatus status = connection.receive(buffer, static_cast<int>(sizeof(buffer)), CANCEL_POLL_MS,
                    received);
                if (status == ReceiveStatus::CLOSED)
                    return ReadStatus::CLOSED;
                if (status == ReceiveStatus::DATA)
                {
                    out.append(buffer, static_cast<std::size_t>(received));
                    return ReadStatus::DATA;
                }
            }
            return ReadStatus::CANCELLED;
        }
    }

    // Подключение → запрос → заголовок → тело / Connecting → the request → the head → the body
    bool performRequest(const HttpRequest& request, HttpReceiver& receiver)
    {
        TcpConnection connection(request.host, request.port);
        std::string bytes;
        HttpHead head;

        connection.sendAll(buildRequestText(request));
        if (readHead(connection, receiver, bytes, head) == http::ReadStatus::CANCELLED)
            return false;
        receiver.onHead(head);
        return http::readBody(connection, head, std::move(bytes), receiver);
    }
}
