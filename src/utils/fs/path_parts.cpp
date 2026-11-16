#include "gran/utils/internal/file_system_detail.hpp"

namespace gran::fs
{
    // Имя — всё после последнего разделителя, но не часть корня («C:» — корень, а не имя)
    // The name is everything after the last separator, but not a part of the root ("C:" is a root, not a name)
    std::string fileName(const std::string& path)
    {
        std::size_t root = internal::rootLength(path);
        std::size_t start = path.size();

        while (start > root && !internal::isPathSeparator(path[start - 1]))
            --start;
        return path.substr(start);
    }

    // Точка в начале имени — часть имени («.profile»), а не расширение; «.» и «..» расширения не имеют
    // A dot at the start of the name is a part of the name (".profile"), not an extension; "." and ".." have no extension
    std::string fileExtension(const std::string& path)
    {
        std::string name = fileName(path);
        std::size_t dot = name.rfind('.');

        if (dot == std::string::npos || dot == 0 || name == "..")
            return std::string();
        return name.substr(dot);
    }

    // Имя без расширения / The name without the extension
    std::string fileStem(const std::string& path)
    {
        std::string name = fileName(path);

        return name.substr(0, name.size() - fileExtension(path).size());
    }

    /*
    ** Родитель: отбрасываются имя и разделители перед ним, корень остаётся целиком.
    ** «a//b» → «a», «/a» → «/», «C:/a» → «C:/», «C:a» → «C:», «a» → «», «/» → «/».
    **
    ** The parent: the name and the separators before it are dropped, the root stays whole.
    ** "a//b" → "a", "/a" → "/", "C:/a" → "C:/", "C:a" → "C:", "a" → "", "/" → "/".
    */
    std::string parentPath(const std::string& path)
    {
        std::size_t root = internal::rootLength(path);
        std::size_t end = path.size() - fileName(path).size();

        while (end > root && internal::isPathSeparator(path[end - 1]))
            --end;
        if (end <= root)
            return internal::rootOf(path);
        return path.substr(0, end);
    }

    // POSIX: корень есть; Windows: корень заканчивается разделителем и это не одиночный «/»
    // POSIX: there is a root; Windows: the root ends with a separator and it is not a single "/"
    bool isAbsolutePath(const std::string& path)
    {
        std::size_t root = internal::rootLength(path);

#if defined(_WIN32)
        return root >= 2 && internal::isPathSeparator(path[root - 1]);
#else
        return root > 0;
#endif
    }
}
