#ifndef CHAT_COMMON_CHAT_ERRORS_HPP
#define CHAT_COMMON_CHAT_ERRORS_HPP

#include "common/chat_texts.hpp"

#include <exception>
#include <initializer_list>
#include <string>
#include <string_view>

namespace chat
{
    /*
    ** Базовое исключение чата: идентификатор текста + готовое сообщение на языке
    ** пользователя (собирается сразу, what() не выделяет память). Наследники различают
    ** источник сбоя, чтобы строка состояния могла сказать «Ollama не отвечает», а не просто «ошибка».
    **
    ** The base chat exception: a text identifier + a ready message in the user language
    ** (built at once, what() allocates nothing). The subclasses tell the failure source apart,
    ** so the status line can say "Ollama does not answer" rather than just "error".
    */
    class ChatException : public std::exception
    {
    public:
        ChatException(ChatText id, std::initializer_list<std::string_view> arguments);

        // Идентификатор текста ошибки / The error text identifier
        ChatText getId() const noexcept;

        // Сообщение на языке пользователя / The message in the user language
        const char* what() const noexcept override;

    private:
        ChatText _id;
        std::string _message;
    };

    // Неверный JSON / Invalid JSON
    class JsonException : public ChatException
    {
    public:
        using ChatException::ChatException;
    };

    // Нарушение протокола HTTP (chunked, заголовок, длина строки) / An HTTP protocol violation (chunked, the head, the line length)
    class ProtocolException : public ChatException
    {
    public:
        using ChatException::ChatException;
    };

    // Сбой сокета: нет связи, отказ, обрыв / A socket failure: no connection, refusal, a break
    class NetworkException : public ChatException
    {
    public:
        using ChatException::ChatException;
    };

    // Сервер ответил не 200 / The server answered not 200
    class HttpException : public ChatException
    {
    public:
        HttpException(int status, std::initializer_list<std::string_view> arguments);

        // Код ответа HTTP / The HTTP status code
        int getStatus() const noexcept;

    private:
        int _status;
    };
}

#endif
