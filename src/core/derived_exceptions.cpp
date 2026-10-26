#include "gran/core/gran_exception.hpp"

#include <utility>

namespace gran
{
    // Ошибка разметки запоминает позицию в исходном файле
    // A markup error remembers the position in the source file
    MarkupException::MarkupException(ErrorCode code, std::string message,
        std::size_t line, std::size_t column)
        : GranException(code, std::move(message)), _line(line), _column(column)
    {
    }

    // Строка (с 1; 0 — неизвестна)
    // The line (from 1; 0 — unknown)
    std::size_t MarkupException::getLine() const noexcept
    {
        return _line;
    }

    // Столбец (с 1; 0 — неизвестен)
    // The column (from 1; 0 — unknown)
    std::size_t MarkupException::getColumn() const noexcept
    {
        return _column;
    }

    // Ошибка GPU хранит «сырой» код HRESULT / VkResult
    // A GPU error keeps the raw HRESULT / VkResult code
    GpuException::GpuException(ErrorCode code, std::string message, long long nativeCode)
        : GranException(code, std::move(message)), _nativeCode(nativeCode)
    {
    }

    // «Сырой» код графического API
    // The raw graphics API code
    long long GpuException::getNativeCode() const noexcept
    {
        return _nativeCode;
    }
}
