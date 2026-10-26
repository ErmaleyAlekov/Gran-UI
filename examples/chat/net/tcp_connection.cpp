#include "net/tcp_connection.hpp"
#include "common/chat_errors.hpp"

// Без макросов min/max из <windows.h> (ломают std::min) и без старого winsock.h / Without the min/max macros of <windows.h> (they break std::min) and without the old winsock.h
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <climits>
#include <string>

namespace chat
{
    namespace
    {
        /*
        ** Перебор адресов getaddrinfo (IPv4 и IPv6) до первого принявшего соединение;
        ** ни один не принял — NetworkException с узлом и портом.
        **
        ** Trying the getaddrinfo addresses (IPv4 and IPv6) until the first one that accepts the connection;
        ** none accepted — NetworkException with the host and the port.
        */
        SOCKET connectTo(const std::string& host, std::uint16_t port)
        {
            addrinfo hints{};
            addrinfo* found = nullptr;
            SOCKET result = INVALID_SOCKET;

            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &found) != 0)
                throw NetworkException(ChatText::NET_CONNECT, {host, std::to_string(port)});
            for (addrinfo* address = found; address != nullptr && result == INVALID_SOCKET; address = address->ai_next)
            {
                result = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
                if (result != INVALID_SOCKET
                    && connect(result, address->ai_addr, static_cast<int>(address->ai_addrlen)) == SOCKET_ERROR)
                {
                    closesocket(result);
                    result = INVALID_SOCKET;
                }
            }
            freeaddrinfo(found);
            if (result == INVALID_SOCKET)
                throw NetworkException(ChatText::NET_CONNECT, {host, std::to_string(port)});
            return result;
        }
    }

    // Сокет открыт и подключён / The socket is open and connected
    TcpConnection::TcpConnection(const std::string& host, std::uint16_t port)
        : _socket(static_cast<std::uintptr_t>(connectTo(host, port)))
    {
    }

    // Закрытие без ожидания: незачитанный хвост ответа не нужен / Closing without waiting: the unread tail of the response is not needed
    TcpConnection::~TcpConnection()
    {
        closesocket(static_cast<SOCKET>(_socket));
    }

    // send возвращает, сколько принял; остаток отправляется следующим вызовом / send returns how much it took; the rest goes with the next call
    void TcpConnection::sendAll(std::string_view data)
    {
        while (!data.empty())
        {
            int part = static_cast<int>(std::min<std::size_t>(data.size(), INT_MAX));
            int sent = send(static_cast<SOCKET>(_socket), data.data(), part, 0);
            if (sent == SOCKET_ERROR)
                throw NetworkException(ChatText::NET_SEND, {std::to_string(WSAGetLastError())});
            data.remove_prefix(static_cast<std::size_t>(sent));
        }
    }

    // select с пределом, затем recv только при готовности: recv уже не блокирует / select with a limit, then recv only when ready: recv no longer blocks
    ReceiveStatus TcpConnection::receive(char* buffer, int size, int timeoutMs, int& received)
    {
        fd_set readable;
        timeval limit{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
        SOCKET handle = static_cast<SOCKET>(_socket);

        received = 0;
        FD_ZERO(&readable);
        FD_SET(handle, &readable);
        int ready = select(0, &readable, nullptr, nullptr, &limit);
        if (ready == SOCKET_ERROR)
            throw NetworkException(ChatText::NET_RECEIVE, {std::to_string(WSAGetLastError())});
        if (ready == 0)
            return ReceiveStatus::TIMEOUT;
        received = recv(handle, buffer, size, 0);
        if (received == SOCKET_ERROR)
            throw NetworkException(ChatText::NET_RECEIVE, {std::to_string(WSAGetLastError())});
        if (received == 0)
            return ReceiveStatus::CLOSED;
        return ReceiveStatus::DATA;
    }
}
