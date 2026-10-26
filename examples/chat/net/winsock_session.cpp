#include "net/tcp_connection.hpp"
#include "common/chat_errors.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>

#include <string>

namespace chat
{
    // WinSock 2.2; код ошибки WSAStartup — в тексте исключения / WinSock 2.2; the WSAStartup error code goes into the exception text
    WinsockSession::WinsockSession()
    {
        WSADATA data;
        int result = WSAStartup(MAKEWORD(2, 2), &data);

        if (result != 0)
            throw NetworkException(ChatText::NET_STARTUP, {std::to_string(result)});
    }

    // Пара к успешному WSAStartup; ошибка при выходе уже ни на что не влияет / The pair of a successful WSAStartup; an error on exit affects nothing anymore
    WinsockSession::~WinsockSession()
    {
        WSACleanup();
    }
}
