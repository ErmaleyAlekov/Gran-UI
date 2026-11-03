#include "gran/res/manifest.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <algorithm>
#include <sstream>

namespace gran::res
{
    namespace
    {
        // Подходит ли имя хотя бы под один шаблон из списка через пробел / Whether the name matches at least one pattern of the space-separated list
        bool matchesAny(const std::string& patterns, const std::string& name)
        {
            std::istringstream list(patterns);
            std::string pattern;

            while (list >> pattern)
            {
                if (matchPattern(pattern, name))
                    return true;
            }
            return false;
        }

        // Порядок по alias (функтор вместо лямбды) / The order by alias (a functor instead of a lambda)
        struct ByAlias
        {
            bool operator()(const ManifestFile& left, const ManifestFile& right) const
            {
                return left.alias < right.alias;
            }
        };
    }

    /*
    ** Жадное сопоставление с откатом к последней «*»: линейно по длине имени для типичных шаблонов,
    ** без рекурсии и без выделений памяти.
    **
    ** Greedy matching with backtracking to the last "*": linear in the name length for typical patterns,
    ** without recursion and without memory allocation.
    */
    bool matchPattern(std::string_view pattern, std::string_view name) noexcept
    {
        std::size_t p = 0;
        std::size_t n = 0;
        std::size_t star = std::string_view::npos;
        std::size_t mark = 0;

        while (n < name.size())
        {
            if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == name[n]))
            {
                ++p;
                ++n;
            }
            else if (p < pattern.size() && pattern[p] == '*')
            {
                star = p++;
                mark = n;
            }
            else if (star != std::string_view::npos)
            {
                p = star + 1;
                n = ++mark;
            }
            else
                return false;
        }
        while (p < pattern.size() && pattern[p] == '*')
            ++p;
        return p == pattern.size();
    }

    // Путь внутри res:/ — alias папки + путь файла относительно неё с косыми «/» / The path inside res:/ — the folder alias + the file path relative to it with "/" slashes
    void expandFolder(const std::filesystem::path& folder, const std::string& alias, const std::string& patterns,
        bool recursive, std::vector<ManifestFile>& files)
    {
        std::error_code error;
        std::filesystem::recursive_directory_iterator entry(folder, error);
        std::filesystem::recursive_directory_iterator end;

        for (; !error && entry != end; entry.increment(error))
        {
            // Без recursive в подпапки не спускаемся / Without recursive the subfolders are not entered
            if (!recursive && entry->is_directory(error))
                entry.disable_recursion_pending();
            if (!entry->is_regular_file(error) || !matchesAny(patterns, entry->path().filename().u8string()))
                continue;
            std::string relative = std::filesystem::relative(entry->path(), folder, error).generic_u8string();
            files.push_back(ManifestFile{entry->path(), alias + "/" + relative});
        }
    }

    // Повтор alias после раскрытия папок — ошибка с самим путём / A repeated alias after expanding the folders is an error with the path itself
    void sortFiles(std::vector<ManifestFile>& files)
    {
        std::sort(files.begin(), files.end(), ByAlias{});
        for (std::size_t index = 1; index < files.size(); ++index)
        {
            if (files[index].alias == files[index - 1].alias)
                throw GranException(ErrorCode::RESOURCE, i18n::format(i18n::Message::RES_DUPLICATE_ALIAS,
                    {files[index].alias}));
        }
    }
}
