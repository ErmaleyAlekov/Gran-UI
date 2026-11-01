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

    /*
    ** Ошибка источника данных (data_exception.cpp): файл JSON или XML, строка и столбец (с 1; 0 —
    ** позиция неизвестна). Текст уже содержит место; поля нужны коду, который показывает ошибку сам.
    **
    ** A data source error (data_exception.cpp): the JSON or XML file, the line and the column (from 1; 0 —
    ** the position is unknown). The text contains the place already; the fields are for code that shows the error itself.
    */
    class DataException : public GranException
    {
    public:
        DataException(std::string message, std::string source, std::size_t line, std::size_t column);

        // Файл источника / The source file
        const std::string& getSource() const noexcept;

        // Строка и столбец в файле / The line and the column in the file
        std::size_t getLine() const noexcept;
        std::size_t getColumn() const noexcept;

    private:
        std::string _source;
        std::size_t _line;
        std::size_t _column;
    };
}

#endif
