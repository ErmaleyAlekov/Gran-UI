#ifndef CHAT_COMMON_HTTP_HEAD_HPP
#define CHAT_COMMON_HTTP_HEAD_HPP

#include <string>
#include <string_view>

namespace chat
{
    // Заголовок ответа HTTP/1.1: то, что нужно для чтения тела / The HTTP/1.1 response head: what is needed to read the body
    struct HttpHead
    {
        int status = 0;
        std::string reason;
        bool chunked = false;
        // Content-Length; -1 — не указан (тело до закрытия соединения) / Content-Length; -1 — not given (the body lasts until the connection closes)
        long long contentLength = -1;
    };

    /*
    ** Разбор заголовка ответа (всё до пустой строки, без неё): строка состояния
    ** «HTTP/1.x код причина», затем поля «Имя: значение» (имена без учёта регистра).
    ** Ошибка — ProtocolException(HTTP_BAD_HEAD).
    **
    ** Parsing the response head (everything up to the empty line, without it): the status line
    ** "HTTP/1.x code reason", then "Name: value" fields (the names are case-insensitive).
    ** An error is ProtocolException(HTTP_BAD_HEAD).
    */
    HttpHead parseHttpHead(std::string_view head);

    // Конец заголовка в накопленных байтах: позиция после «\r\n\r\n» (или «\n\n»); npos — ещё не пришёл
    // The head end in the accumulated bytes: the position after "\r\n\r\n" (or "\n\n"); npos — not arrived yet
    std::size_t findHeadEnd(std::string_view bytes) noexcept;
}

#endif
