#include "common/chat_errors.hpp"

namespace chat
{
    // Текст собирается сразу на текущем языке / The text is built at once in the current language
    ChatException::ChatException(ChatText id, std::initializer_list<std::string_view> arguments)
        : _id(id), _message(say(id, arguments))
    {
    }

    // Идентификатор текста / The text identifier
    ChatText ChatException::getId() const noexcept
    {
        return _id;
    }

    // Готовая строка / The ready string
    const char* ChatException::what() const noexcept
    {
        return _message.c_str();
    }

    // Аргументы — код и причина ответа для текста HTTP_STATUS / The arguments are the status code and reason for the HTTP_STATUS text
    HttpException::HttpException(int status, std::initializer_list<std::string_view> arguments)
        : ChatException(ChatText::HTTP_STATUS, arguments), _status(status)
    {
    }

    // Код ответа / The status code
    int HttpException::getStatus() const noexcept
    {
        return _status;
    }
}
