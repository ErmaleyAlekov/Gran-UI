#ifndef GRAN_CORE_GRAN_EXCEPTION_HPP
#define GRAN_CORE_GRAN_EXCEPTION_HPP

#include "gran/core/error_code.hpp"

#include <cstddef>
#include <exception>
#include <string>

namespace gran
{
    /*
    ** Базовое исключение движка: код + сообщение. what() возвращает заранее
    ** собранную строку "[КОД] сообщение", чтобы не выделять память при обработке.
    **
    ** The base engine exception: a code + a message. what() returns a pre-built
    ** "[CODE] message" string so that no memory is allocated while it is handled.
    */
    class GranException : public std::exception
    {
    public:
        GranException(ErrorCode code, std::string message);

        // Категория ошибки / The error category
        ErrorCode getCode() const noexcept;

        // Сообщение без префикса кода / The message without the code prefix
        const std::string& getMessage() const noexcept;

        // Полный текст "[КОД] сообщение" / The full "[CODE] message" text
        const char* what() const noexcept override;

    private:
        ErrorCode _code;
        std::string _message;
        std::string _fullText;
    };

    /*
    ** Ошибка в разметке с позицией: строка и столбец (с 1; 0 — позиция неизвестна).
    ** Ошибки схемы XSD приходят уже с позицией и путём от «Берёсты».
    **
    ** A markup error with a position: line and column (from 1; 0 — the position is unknown).
    ** XSD schema errors arrive with the position and the path from Beresta already.
    */
    class MarkupException : public GranException
    {
    public:
        MarkupException(ErrorCode code, std::string message, std::size_t line, std::size_t column);

        // Строка в файле разметки / The line in the markup file
        std::size_t getLine() const noexcept;

        // Столбец в файле разметки / The column in the markup file
        std::size_t getColumn() const noexcept;

    private:
        std::size_t _line;
        std::size_t _column;
    };

    /*
    ** Ошибка графического API: хранит «сырой» код (HRESULT или VkResult),
    ** чтобы его можно было сверить с документацией драйвера.
    **
    ** A graphics API error: keeps the raw code (HRESULT or VkResult)
    ** so it can be checked against the driver documentation.
    */
    class GpuException : public GranException
    {
    public:
        GpuException(ErrorCode code, std::string message, long long nativeCode);

        // Код HRESULT / VkResult / The HRESULT / VkResult code
        long long getNativeCode() const noexcept;

    private:
        long long _nativeCode;
    };
}

#endif
