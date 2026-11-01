#include "gran/data/json_reader.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <utility>

namespace gran::data
{
    namespace
    {
        // Размер куска чтения файла / The file read chunk size
        constexpr std::size_t CHUNK = 64 * 1024;
    }

    JsonReader::JsonReader(std::istream& input, std::string source)
        : _input(input), _source(std::move(source)), _buffer(CHUNK)
    {
    }

    /*
    ** Вне контейнеров ровно одно значение верхнего уровня (метка UTF-8 BOM в начале пропускается),
    ** после него — только пробелы до конца файла.
    **
    ** Outside containers there is exactly one top level value (a UTF-8 BOM mark at the start is skipped),
    ** after it — only whitespace up to the end of file.
    */
    JsonToken JsonReader::next()
    {
        if (!_frames.empty() && _frames.back().object)
            return nextInObject(_frames.back());
        if (!_frames.empty())
            return nextInArray(_frames.back());
        if (!_started)
        {
            _started = true;
            if (peek() == 0xEF)
            {
                take();
                take();
                take();
            }
            return readValue();
        }
        skipSpace();
        if (peek() != -1)
            unexpected();
        return JsonToken::END;
    }

    const std::string& JsonReader::value() const noexcept
    {
        return _value;
    }

    // Контейнер пропускается, пока число открытых не станет меньше, чем было при его начале / A container is skipped until the open count gets less than at its start
    void JsonReader::skip(JsonToken token)
    {
        std::size_t depth = _frames.size();

        if (token != JsonToken::BEGIN_OBJECT && token != JsonToken::BEGIN_ARRAY)
            return;
        while (_frames.size() >= depth)
        {
            if (next() == JsonToken::END)
                return;
        }
    }

    void JsonReader::fail(const std::string& detail) const
    {
        throw DataException(i18n::format(i18n::Message::DATA_SOURCE_ERROR,
            {_source, std::to_string(_line), std::to_string(_column), detail}), _source, _line, _column);
    }
}
