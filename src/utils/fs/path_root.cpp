#include "gran/utils/internal/file_system_detail.hpp"

namespace gran::fs::internal
{
    // На POSIX «\» — обычный символ имени файла, на Windows — разделитель, как «/»
    // On POSIX "\" is an ordinary file name symbol, on Windows it is a separator, like "/"
    bool isPathSeparator(char symbol) noexcept
    {
#if defined(_WIN32)
        return symbol == '/' || symbol == '\\';
#else
        return symbol == '/';
#endif
    }

    /*
    ** Корень пути. POSIX: ведущие «/» (сколько бы их ни было). Windows: буква диска с двоеточием
    ** («C:», затем разделитель, если есть) или сетевой путь «//сервер/» (два разделителя, имя, разделитель),
    ** иначе одиночный ведущий разделитель («/a» — корень текущего диска).
    **
    ** The path root. POSIX: the leading "/" (however many). Windows: a drive letter with a colon
    ** ("C:", then a separator if present) or a network path "//server/" (two separators, a name, a separator),
    ** otherwise a single leading separator ("/a" is the root of the current drive).
    */
    std::size_t rootLength(const std::string& path) noexcept
    {
        std::size_t length = 0;

#if defined(_WIN32)
        bool letter = path.size() >= 2 && path[1] == ':'
            && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z'));
        if (letter)
            length = 2;
        else if (path.size() > 2 && isPathSeparator(path[0]) && isPathSeparator(path[1]) && !isPathSeparator(path[2]))
        {
            // Сетевой путь: имя сервера до следующего разделителя / A network path: the server name up to the next separator
            length = 2;
            while (length < path.size() && !isPathSeparator(path[length]))
                ++length;
        }
        if (length < path.size() && isPathSeparator(path[length]))
            ++length;
#else
        while (length < path.size() && path[length] == '/')
            ++length;
#endif
        return length;
    }

    // Корень с «/» вместо «\» и без повторов «/» в конце / The root with "/" instead of "\" and no repeated trailing "/"
    std::string rootOf(const std::string& path)
    {
        std::string root = path.substr(0, rootLength(path));

        for (char& symbol : root)
        {
            if (isPathSeparator(symbol))
                symbol = '/';
        }
#if !defined(_WIN32)
        // POSIX: «///a» — тот же корень «/» / POSIX: "///a" has the same root "/"
        if (!root.empty())
            root = "/";
#endif
        return root;
    }

    // Корень без завершающего разделителя; на POSIX имени корня нет / The root without the trailing separator; POSIX has no root name
    std::string rootName(const std::string& path)
    {
        std::string root;

#if defined(_WIN32)
        root = rootOf(path);
        if (!root.empty() && root.back() == '/')
            root.pop_back();
        // Одиночный «/» — корень текущего диска без имени / A single "/" is the current drive root without a name
#else
        (void)path;
#endif
        return root;
    }
}
