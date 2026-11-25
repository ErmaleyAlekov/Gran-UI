#include "common/markdown_internal.hpp"

namespace chat::markdown
{
    bool markerSpace(char symbol) noexcept
    {
        return symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r';
    }

    // Один байт — литеральная кавычка. / One byte denotes a literal backtick.
    std::size_t codeSpanLength(std::string_view text)
    {
        auto end = text.find('`', 1);
        return end == text.npos || end == 1 ? 1 : end + 1;
    }

    ClosingMarkers closingMarkers(std::string_view text)
    {
        ClosingMarkers markers;
        for (std::size_t pos = 0; pos < text.size();)
        {
            if (text[pos] == '`')
                pos += codeSpanLength(text.substr(pos));
            else if (text[pos] == '*')
            {
                bool bold = pos + 1 < text.size() && text[pos + 1] == '*';
                if (pos > 0 && !markerSpace(text[pos - 1]))
                    (bold ? markers.bold : markers.italic) = pos;
                pos += bold ? 2 : 1;
            }
            else
                ++pos;
        }
        return markers;
    }
}
