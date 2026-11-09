#include "gran/utils/uri_list.hpp"

#include <cctype>

namespace gran
{
    namespace
    {
        // Значение шестнадцатеричной цифры; -1 — не цифра / The hexadecimal digit value; -1 — not a digit
        int hexValue(char symbol) noexcept
        {
            if (symbol >= '0' && symbol <= '9')
                return symbol - '0';
            if (symbol >= 'a' && symbol <= 'f')
                return symbol - 'a' + 10;
            if (symbol >= 'A' && symbol <= 'F')
                return symbol - 'A' + 10;
            return -1;
        }

        // Строки равны без учёта регистра ASCII / The strings are equal ignoring the ASCII case
        bool equalsIgnoringCase(std::string_view text, std::string_view expected) noexcept
        {
            if (text.size() != expected.size())
                return false;
            for (std::size_t index = 0; index < text.size(); ++index)
            {
                unsigned char left = static_cast<unsigned char>(text[index]);
                unsigned char right = static_cast<unsigned char>(expected[index]);
                if (std::tolower(left) != std::tolower(right))
                    return false;
            }
            return true;
        }
    }

    // Каждый `%` обязан нести две шестнадцатеричные цифры; `%00` запрещён / Every `%` must carry two hexadecimal digits; `%00` is forbidden
    bool percentDecode(std::string_view text, std::string& out)
    {
        out.clear();
        for (std::size_t index = 0; index < text.size(); ++index)
        {
            if (text[index] != '%')
            {
                out += text[index];
                continue;
            }
            if (index + 2 >= text.size())
                return false;
            int high = hexValue(text[index + 1]);
            int low = hexValue(text[index + 2]);
            if (high < 0 || low < 0 || (high == 0 && low == 0))
                return false;
            out += static_cast<char>(high * 16 + low);
            index += 2;
        }
        return true;
    }

    /*
    ** Схема `file:` (без учёта регистра), затем необязательный узел `//узел` — пустой или localhost;
    ** путь обязан начинаться с `/`. Запрос `?` и фрагмент `#` к пути не относятся и отрезаются.
    **
    ** The `file:` scheme (ignoring the case), then an optional `//host` — empty or localhost;
    ** the path must start with `/`. The `?` query and the `#` fragment do not belong to the path and are cut off.
    */
    bool fileUriPath(std::string_view uri, std::string& path)
    {
        if (uri.size() < 5 || !equalsIgnoringCase(uri.substr(0, 5), "file:"))
            return false;
        std::string_view rest = uri.substr(5);
        if (rest.size() >= 2 && rest[0] == '/' && rest[1] == '/')
        {
            std::size_t slash = rest.find('/', 2);
            if (slash == std::string_view::npos)
                return false;
            std::string_view host = rest.substr(2, slash - 2);
            if (!host.empty() && !equalsIgnoringCase(host, "localhost"))
                return false;
            rest = rest.substr(slash);
        }
        if (rest.empty() || rest[0] != '/')
            return false;
        std::size_t tail = rest.find_first_of("?#");
        if (tail != std::string_view::npos)
            rest = rest.substr(0, tail);
        return percentDecode(rest, path);
    }

    // Строки по LF с отрезанием CR; комментарии и не-файловые адреса пропускаются / Lines by LF with CR cut off; comments and non-file addresses are skipped
    std::vector<std::string> parseUriList(std::string_view text)
    {
        std::vector<std::string> paths;
        std::size_t start = 0;

        while (start < text.size())
        {
            std::size_t end = text.find('\n', start);
            if (end == std::string_view::npos)
                end = text.size();
            std::string_view line = text.substr(start, end - start);
            start = end + 1;
            if (!line.empty() && line.back() == '\r')
                line.remove_suffix(1);
            std::string path;
            if (!line.empty() && line.front() != '#' && fileUriPath(line, path))
                paths.push_back(path);
        }
        return paths;
    }
}
