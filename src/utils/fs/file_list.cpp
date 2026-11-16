#include "gran/utils/internal/file_system_detail.hpp"

#include <algorithm>

namespace gran::fs
{
    namespace
    {
        /*
        ** Обход одной папки: prefix — её путь относительно корня обхода («» у самого корня). В папки-ссылки
        ** не спускаемся: ссылка на родителя иначе зациклила бы обход (так же ведёт себя
        ** std::filesystem::recursive_directory_iterator по умолчанию).
        **
        ** Walking one folder: prefix is its path relative to the walk root ("" for the root itself). Link folders
        ** are not entered: a link to a parent would otherwise loop the walk (std::filesystem::recursive_directory_iterator
        ** behaves the same way by default).
        */
        void walkFolder(const std::string& root, const std::string& prefix, bool recursive, std::vector<std::string>& files)
        {
            for (const DirectoryEntry& entry : listDirectory(joinPath(root, prefix)))
            {
                std::string relative = entry.name;
                if (!prefix.empty())
                    relative = prefix + "/" + entry.name;
                if (entry.info.directory)
                {
                    if (recursive && !entry.info.link)
                        walkFolder(root, relative, recursive, files);
                }
                else if (entry.info.regular)
                    files.push_back(relative);
            }
        }
    }

    // Порядок ОС у readdir и FindFirstFile разный — сортировка делает итог одинаковым на всех ОС
    // The OS order of readdir and FindFirstFile differs — sorting makes the result the same on every OS
    std::vector<std::string> listFiles(const std::string& root, bool recursive)
    {
        std::vector<std::string> files;

        walkFolder(root, std::string(), recursive, files);
        std::sort(files.begin(), files.end());
        return files;
    }
}
