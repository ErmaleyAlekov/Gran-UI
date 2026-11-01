#include "gran/core/gran_exception.hpp"

#include <utility>

namespace gran
{
    DataException::DataException(std::string message, std::string source, std::size_t line, std::size_t column)
        : GranException(ErrorCode::DATA, std::move(message)), _source(std::move(source)), _line(line), _column(column)
    {
    }

    const std::string& DataException::getSource() const noexcept
    {
        return _source;
    }

    std::size_t DataException::getLine() const noexcept
    {
        return _line;
    }

    std::size_t DataException::getColumn() const noexcept
    {
        return _column;
    }
}
