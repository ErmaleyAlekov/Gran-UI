#include "net/http_internal.hpp"
#include "common/chat_errors.hpp"

#include <algorithm>
#include <utility>

namespace chat::http
{
    // chunked декодируется, Content-Length отсчитывается (лишнее после тела отбрасывается) / chunked is decoded, Content-Length is counted down (the extra after the body is dropped)
    void BodyFraming::feed(const std::string& raw, HttpReceiver& receiver)
    {
        if (head.chunked)
        {
            std::string payload;
            decoder.feed(raw, payload);
            if (!payload.empty())
                receiver.onBody(payload);
            return;
        }
        std::string_view piece(raw);
        if (remaining >= 0)
        {
            piece = piece.substr(0, static_cast<std::size_t>(std::min<long long>(remaining,
                static_cast<long long>(piece.size()))));
            remaining -= static_cast<long long>(piece.size());
        }
        if (!piece.empty())
            receiver.onBody(piece);
    }

    // Без chunked и без Content-Length тело кончается только закрытием соединения / Without chunked and without Content-Length the body ends only with the connection close
    bool BodyFraming::finished() const noexcept
    {
        if (head.chunked)
            return decoder.finished();
        return remaining == 0;
    }

    /*
    ** Сначала байты, пришедшие вместе с заголовком, затем чтение до конца тела.
    ** Закрытие соединения раньше конца — обрыв (NET_CLOSED), кроме тела «до закрытия».
    **
    ** First the bytes that arrived with the head, then reading until the body end.
    ** A connection close before the end is a break (NET_CLOSED), except for an "until close" body.
    */
    bool readBody(TcpConnection& connection, const HttpHead& head, std::string pending, HttpReceiver& receiver)
    {
        BodyFraming framing{head, ChunkedDecoder{}, head.contentLength};

        while (true)
        {
            if (!pending.empty())
                framing.feed(pending, receiver);
            pending.clear();
            if (framing.finished())
                return true;
            ReadStatus status = readMore(connection, receiver, pending);
            if (status == ReadStatus::CANCELLED)
                return false;
            if (status == ReadStatus::CLOSED && !head.chunked && head.contentLength < 0)
                return true;
            if (status == ReadStatus::CLOSED)
                throw NetworkException(ChatText::NET_CLOSED, {});
        }
    }
}
