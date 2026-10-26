#include "common/line_splitter.hpp"
#include "common/chat_errors.hpp"

#include <utility>

namespace chat
{
    // Предел запоминается / The limit is remembered
    LineSplitter::LineSplitter(std::size_t maxLine)
        : _maxLine(maxLine)
    {
    }

    // Строка собирается в _pending, на LF отдаётся без CR / A line is collected in _pending, on LF it is given away without CR
    void LineSplitter::feed(std::string_view data, std::vector<std::string>& lines)
    {
        std::size_t position = 0;

        while (position < data.size())
        {
            std::size_t end = data.find('\n', position);
            std::size_t stop = end;
            if (stop == std::string_view::npos)
                stop = data.size();
            _pending.append(data.substr(position, stop - position));
            if (_pending.size() > _maxLine)
                throw ProtocolException(ChatText::LINE_TOO_LONG, {std::to_string(_maxLine)});
            if (end == std::string_view::npos)
                return;
            position = end + 1;
            if (!_pending.empty() && _pending.back() == '\r')
                _pending.pop_back();
            if (!_pending.empty())
                lines.push_back(std::move(_pending));
            _pending.clear();
        }
    }

    // Остаток отдаётся и очищается / The rest is given away and cleared
    std::string LineSplitter::flush()
    {
        std::string rest = std::move(_pending);

        _pending.clear();
        if (!rest.empty() && rest.back() == '\r')
            rest.pop_back();
        return rest;
    }
}
