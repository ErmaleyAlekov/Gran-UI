#include "gran/utils/internal/file_system_detail.hpp"

#include <vector>

namespace gran::fs
{
    namespace
    {
        // Элементы пути после корня без пустых и «.» / The path elements after the root without empty ones and "."
        std::vector<std::string> splitElements(const std::string& path, std::size_t root)
        {
            std::vector<std::string> elements;
            std::string element;

            for (std::size_t index = root; index <= path.size(); ++index)
            {
                if (index < path.size() && !internal::isPathSeparator(path[index]))
                {
                    element += path[index];
                    continue;
                }
                if (!element.empty() && element != ".")
                    elements.push_back(element);
                element.clear();
            }
            return elements;
        }
    }

    /*
    ** Склейка. Абсолютный tail заменяет base. На Windows tail с одним ведущим разделителем («/a») — от имени корня
    ** base («C:» или «//сервер»: «//srv/share» + «/sub» → «//srv/sub», как у std::filesystem; сетевой корень терялся —
    ** ревью Т-57), а «C:a» с буквой диска — сам по себе. Разделитель добавляется, только если base
    ** им не кончается и это не одна буква диска («C:» + «a» → «C:a», как у std::filesystem).
    **
    ** Joining. An absolute tail replaces base. On Windows a tail with one leading separator ("/a") goes from the base root
    ** name ("C:" or "//server": "//srv/share" + "/sub" → "//srv/sub", as in std::filesystem; the network root was lost —
    ** the T-57 review), and "C:a" with a drive letter stands by itself. The separator is added only if base
    ** does not end with one and it is not a bare drive letter ("C:" + "a" → "C:a", as in std::filesystem).
    */
    std::string joinPath(const std::string& base, const std::string& tail)
    {
        if (tail.empty())
            return base;
        if (base.empty() || isAbsolutePath(tail))
            return tail;
        if (internal::rootLength(tail) > 0)
        {
            // Только Windows: на POSIX корень у tail означает абсолютный путь (ветка выше)
            // Windows only: on POSIX a tail root means an absolute path (the branch above)
            if (internal::isPathSeparator(tail[0]))
                return internal::rootName(base) + tail;
            return tail;
        }
        if (internal::isPathSeparator(base.back()) || internal::rootLength(base) == base.size())
            return base + tail;
        return base + "/" + tail;
    }

    /*
    ** Нормализация без диска: «..» снимает предыдущий элемент; выше корня («/..») подняться нельзя —
    ** такой «..» отбрасывается; у относительного пути ведущие «..» остаются. Пустой итог — «.».
    **
    ** Normalization without the disk: ".." removes the previous element; going above the root ("/..") is impossible —
    ** such ".." is dropped; a relative path keeps its leading "..". An empty result is ".".
    */
    std::string normalizePath(const std::string& path)
    {
        std::string root = internal::rootOf(path);
        bool rooted = !root.empty() && root.back() == '/';
        std::vector<std::string> kept;
        std::string result = root;

        for (const std::string& element : splitElements(path, internal::rootLength(path)))
        {
            if (element != "..")
                kept.push_back(element);
            else if (!kept.empty() && kept.back() != "..")
                kept.pop_back();
            else if (!rooted)
                kept.push_back(element);
        }
        for (std::size_t index = 0; index < kept.size(); ++index)
        {
            if (index > 0)
                result += '/';
            result += kept[index];
        }
        if (result.empty() && !path.empty())
            return ".";
        return result;
    }

    // Относительный путь — от текущей папки процесса / A relative path goes from the current process folder
    std::string absolutePath(const std::string& path)
    {
        if (isAbsolutePath(path))
            return normalizePath(path);
        return normalizePath(joinPath(currentDirectory(), path));
    }
}
