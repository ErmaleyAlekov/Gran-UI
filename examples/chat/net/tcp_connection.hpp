#ifndef CHAT_NET_TCP_CONNECTION_HPP
#define CHAT_NET_TCP_CONNECTION_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace chat
{
    /*
    ** Запуск и остановка WinSock (RAII): один объект на программу, живёт дольше всех
    ** соединений. Ошибка запуска — NetworkException.
    **
    ** Starting and stopping WinSock (RAII): one object per program, it outlives all
    ** connections. A startup error is NetworkException.
    */
    class WinsockSession
    {
    public:
        // ---- winsock_session.cpp ----
        WinsockSession();
        ~WinsockSession();
        WinsockSession(const WinsockSession&) = delete;
        WinsockSession& operator=(const WinsockSession&) = delete;
    };

    // Итог ожидания данных / The outcome of waiting for data
    enum class ReceiveStatus : std::uint8_t { DATA, CLOSED, TIMEOUT };

    /*
    ** Соединение TCP (RAII: сокет закрывается деструктором). Ожидание данных — через select
    ** с пределом времени: поток может между ожиданиями проверить флаг отмены, а сокет при этом
    ** остаётся исправным (таймаут SO_RCVTIMEO в Windows оставил бы его в неопределённом состоянии).
    **
    ** A TCP connection (RAII: the socket is closed by the destructor). Waiting for data goes through select
    ** with a time limit: the thread can check the cancel flag between waits, while the socket
    ** stays healthy (an SO_RCVTIMEO timeout on Windows would leave it in an undefined state).
    */
    class TcpConnection
    {
    public:
        // ---- tcp_connection.cpp ----

        // Подключение к узлу (имя или адрес) и порту; отказ — NetworkException / Connecting to a host (a name or an address) and a port; a refusal — NetworkException
        TcpConnection(const std::string& host, std::uint16_t port);
        ~TcpConnection();
        TcpConnection(const TcpConnection&) = delete;
        TcpConnection& operator=(const TcpConnection&) = delete;

        // Отправка всех байтов (send может принять только часть) / Sending all bytes (send may take only a part)
        void sendAll(std::string_view data);

        // Ждать данных не дольше timeoutMs; при DATA в received — число байтов / Wait for data at most timeoutMs; on DATA received holds the byte count
        ReceiveStatus receive(char* buffer, int size, int timeoutMs, int& received);

    private:
        // SOCKET хранится как целое: заголовок не тянет winsock2.h / SOCKET is stored as an integer: the header does not pull winsock2.h
        std::uintptr_t _socket;
    };
}

#endif
