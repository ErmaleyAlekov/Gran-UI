#include "gran/utils/internal/file_system_detail.hpp"

namespace gran::fs
{
    // Существует ли путь (висячая ссылка — нет) / Whether the path exists (a dangling link does not)
    bool pathExists(const std::string& path)
    {
        return queryPath(path).exists;
    }

    // Обычный файл (ссылка на файл — тоже) / A regular file (a link to a file as well)
    bool isRegularFile(const std::string& path)
    {
        return queryPath(path).regular;
    }

    // Папка (ссылка на папку — тоже) / A folder (a link to a folder as well)
    bool isDirectory(const std::string& path)
    {
        return queryPath(path).directory;
    }

    /*
    ** Сначала родители, затем сама папка. Глубина рекурсии — число элементов пути: родитель всегда короче,
    ** а на корне («/», «C:/») или пустом родителе рекурсия останавливается.
    **
    ** The parents first, then the folder itself. The recursion depth is the number of path elements: a parent is always
    ** shorter, and the recursion stops at the root ("/", "C:/") or an empty parent.
    */
    bool createDirectories(const std::string& path)
    {
        std::string parent = parentPath(path);

        if (path.empty())
            return false;
        if (isDirectory(path))
            return true;
        if (!parent.empty() && parent.size() < path.size() && !createDirectories(parent))
            return false;
        return createDirectory(path);
    }
}
