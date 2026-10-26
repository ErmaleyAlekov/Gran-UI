#include "common/http_head.hpp"
#include "common/chat_errors.hpp"
#include "common/text_utils.hpp"

namespace chat
{
    namespace
    {
        // «HTTP/1.1 200 OK»: версия HTTP/1.x, ровно три цифры кода, причина может быть пустой / "HTTP/1.1 200 OK": the HTTP/1.x version, exactly three code digits, the reason may be empty
        void parseStatusLine(std::string_view line, HttpHead& head)
        {
            if (line.size() < 12 || line.substr(0, 7) != "HTTP/1." || line[8] != ' ')
                throw ProtocolException(ChatText::HTTP_BAD_HEAD, {});
            head.status = 0;
            for (std::size_t index = 9; index < 12; ++index)
            {
                if (line[index] < '0' || line[index] > '9')
                    throw ProtocolException(ChatText::HTTP_BAD_HEAD, {});
                head.status = head.status * 10 + (line[index] - '0');
            }
            if (line.size() > 12 && line[12] != ' ')
                throw ProtocolException(ChatText::HTTP_BAD_HEAD, {});
            if (line.size() > 13)
                head.reason = std::string(trimSpaces(line.substr(13)));
        }

        // Нужны только Transfer-Encoding и Content-Length; остальные поля пропускаются / Only Transfer-Encoding and Content-Length are needed; the other fields are skipped
        void parseField(std::string_view line, HttpHead& head)
        {
            std::size_t colon = line.find(':');

            if (colon == std::string_view::npos || colon == 0)
                throw ProtocolException(ChatText::HTTP_BAD_HEAD, {});
            std::string_view name = line.substr(0, colon);
            std::string_view value = trimSpaces(line.substr(colon + 1));
            if (equalsIgnoreCase(name, "Transfer-Encoding"))
                head.chunked = value.size() >= 7 && equalsIgnoreCase(value.substr(value.size() - 7), "chunked");
            else if (equalsIgnoreCase(name, "Content-Length"))
            {
                head.contentLength = 0;
                for (char symbol : value)
                {
                    if (symbol < '0' || symbol > '9' || head.contentLength > 1000000000000LL)
                        throw ProtocolException(ChatText::HTTP_BAD_HEAD, {});
                    head.contentLength = head.contentLength * 10 + (symbol - '0');
                }
            }
        }
    }

    // Строки делятся по LF, CR в конце отбрасывается / The lines are split by LF, a trailing CR is dropped
    HttpHead parseHttpHead(std::string_view head)
    {
        HttpHead result;
        bool first = true;

        while (!head.empty())
        {
            std::size_t end = head.find('\n');
            std::string_view line = head.substr(0, end);
            head.remove_prefix(line.size());
            if (!head.empty())
                head.remove_prefix(1);
            if (!line.empty() && line.back() == '\r')
                line.remove_suffix(1);
            if (first)
                parseStatusLine(line, result);
            else if (!line.empty())
                parseField(line, result);
            first = false;
        }
        if (first)
            throw ProtocolException(ChatText::HTTP_BAD_HEAD, {});
        return result;
    }

    // Строгий CRLF CRLF или терпимый LF LF — что встретится раньше / The strict CRLF CRLF or the tolerant LF LF — whichever comes first
    std::size_t findHeadEnd(std::string_view bytes) noexcept
    {
        std::size_t strict = bytes.find("\r\n\r\n");
        std::size_t loose = bytes.find("\n\n");
        std::size_t best = std::string_view::npos;

        if (strict != std::string_view::npos)
            best = strict + 4;
        if (loose != std::string_view::npos && (best == std::string_view::npos || loose + 2 < best))
            best = loose + 2;
        return best;
    }
}
