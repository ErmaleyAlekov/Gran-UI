#include "gran/core/gran_exception.hpp"

#include <utility>

namespace gran
{
    // Полный текст собирается сразу: what() не должен выделять память
    // The full text is built at once: what() must not allocate memory
    GranException::GranException(ErrorCode code, std::string message)
        : _code(code), _message(std::move(message))
    {
        _fullText = std::string("[") + errorCodeName(code) + "] " + _message;
    }

    // Категория ошибки
    // The error category
    ErrorCode GranException::getCode() const noexcept
    {
        return _code;
    }

    // Сообщение без префикса
    // The message without the prefix
    const std::string& GranException::getMessage() const noexcept
    {
        return _message;
    }

    // Готовая строка "[КОД] сообщение"
    // The ready "[CODE] message" string
    const char* GranException::what() const noexcept
    {
        return _fullText.c_str();
    }
}
