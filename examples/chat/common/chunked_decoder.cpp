#include "common/chunked_decoder.hpp"
#include "common/chat_errors.hpp"
#include "common/text_utils.hpp"

#include <algorithm>
#include <string>

namespace chat
{
    namespace
    {
        /*
        ** Размер блока — шестнадцатеричное число до «;» (расширения отбрасываются) и без пробелов
        ** по краям; больше 15 цифр (2^60) — заведомо ошибка, а не повод переполнить счётчик.
        **
        ** The block size is a hex number up to ";" (the extensions are dropped) without spaces
        ** around it; more than 15 digits (2^60) is certainly an error, not a reason to overflow the counter.
        */
        std::uint64_t parseSize(const std::string& line)
        {
            std::string_view digits = trimSpaces(std::string_view(line).substr(0, line.find(';')));
            std::uint64_t size = 0;

            if (digits.empty() || digits.size() > 15)
                throw ProtocolException(ChatText::CHUNK_BAD_SIZE, {line});
            for (char symbol : digits)
            {
                int digit = hexDigit(symbol);
                if (digit < 0)
                    throw ProtocolException(ChatText::CHUNK_BAD_SIZE, {line});
                size = size * 16 + static_cast<std::uint64_t>(digit);
            }
            return size;
        }
    }

    // Строка может прийти в нескольких кусках: _line копит её между вызовами / A line may arrive in several pieces: _line accumulates it between calls
    bool ChunkedDecoder::takeLine(std::string_view data, std::size_t& position)
    {
        std::size_t end = data.find('\n', position);
        std::size_t stop = end;

        if (stop == std::string_view::npos)
            stop = data.size();
        _line.append(data.substr(position, stop - position));
        if (_line.size() > CHUNK_MAX_LINE)
            throw ProtocolException(ChatText::LINE_TOO_LONG, {std::to_string(CHUNK_MAX_LINE)});
        if (end == std::string_view::npos)
        {
            position = data.size();
            return false;
        }
        position = end + 1;
        if (!_line.empty() && _line.back() == '\r')
            _line.pop_back();
        return true;
    }

    // SIZE → DATA (или TRAILER для нуля), DATA_END → SIZE, TRAILER до пустой строки → DONE / SIZE → DATA (or TRAILER for zero), DATA_END → SIZE, TRAILER until an empty line → DONE
    void ChunkedDecoder::lineDone()
    {
        if (_phase == Phase::SIZE)
        {
            _remaining = parseSize(_line);
            _phase = Phase::DATA;
            if (_remaining == 0)
                _phase = Phase::TRAILER;
        }
        else if (_phase == Phase::DATA_END)
        {
            if (!_line.empty())
                throw ProtocolException(ChatText::CHUNK_BAD_END, {});
            _phase = Phase::SIZE;
        }
        else if (_phase == Phase::TRAILER && _line.empty())
            _phase = Phase::DONE;
        _line.clear();
    }

    // Данные копируются кусками без разбора, служебные строки — через takeLine / The data is copied in pieces without parsing, the service lines go through takeLine
    void ChunkedDecoder::feed(std::string_view data, std::string& out)
    {
        std::size_t position = 0;

        while (position < data.size() && _phase != Phase::DONE)
        {
            // После данных блока допустим только CRLF: иной байт — ошибка сразу, не дожидаясь LF
            // Only CRLF may follow the block data: another byte is an error at once, without waiting for LF
            if (_phase == Phase::DATA_END && data[position] != '\r' && data[position] != '\n')
                throw ProtocolException(ChatText::CHUNK_BAD_END, {});
            if (_phase != Phase::DATA)
            {
                if (takeLine(data, position))
                    lineDone();
                continue;
            }
            std::size_t count = static_cast<std::size_t>(
                std::min<std::uint64_t>(_remaining, data.size() - position));
            out.append(data.substr(position, count));
            position += count;
            _remaining -= count;
            if (_remaining == 0)
                _phase = Phase::DATA_END;
        }
    }

    // Конец потока / The stream end
    bool ChunkedDecoder::finished() const noexcept
    {
        return _phase == Phase::DONE;
    }
}
