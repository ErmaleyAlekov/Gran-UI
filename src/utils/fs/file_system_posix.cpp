#if !defined(_WIN32)

#include "gran/utils/internal/file_system_detail.hpp"

#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace gran::fs
{
    /*
    ** lstat — ссылка ли сам путь; stat — сведения о цели (висячая ссылка — пути нет, как у std::filesystem::exists).
    ** Время — st_mtim (Linux) или st_mtimespec (macOS) с наносекундами.
    **
    ** lstat tells whether the path itself is a link; stat gives the target information (a dangling link means no path,
    ** as in std::filesystem::exists). The time is st_mtim (Linux) or st_mtimespec (macOS) with nanoseconds.
    */
    FileInfo queryPath(const std::string& path) noexcept
    {
        struct stat own{};
        struct stat target{};
        FileInfo info;

        if (path.empty() || ::lstat(path.c_str(), &own) != 0)
            return info;
        info.link = S_ISLNK(own.st_mode);
        if (::stat(path.c_str(), &target) != 0)
            return info;
        info.exists = true;
        info.directory = S_ISDIR(target.st_mode);
        info.regular = S_ISREG(target.st_mode);
        if (info.regular && target.st_size > 0)
            info.size = static_cast<std::uint64_t>(target.st_size);
#if defined(__APPLE__)
        info.modified = static_cast<std::int64_t>(target.st_mtimespec.tv_sec) * 1000000000LL + target.st_mtimespec.tv_nsec;
#else
        info.modified = static_cast<std::int64_t>(target.st_mtim.tv_sec) * 1000000000LL + target.st_mtim.tv_nsec;
#endif
        return info;
    }

    // readdir без d_type: сведения о каждом элементе — queryPath (d_type есть не на всех файловых системах)
    // readdir without d_type: the information about each entry is queryPath (d_type is not on every file system)
    std::vector<DirectoryEntry> listDirectory(const std::string& path)
    {
        std::vector<DirectoryEntry> entries;
        DIR* folder = nullptr;

        if (!path.empty())
            folder = ::opendir(path.c_str());
        if (folder == nullptr)
            return entries;
        try
        {
            for (dirent* entry = ::readdir(folder); entry != nullptr; entry = ::readdir(folder))
            {
                std::string name = entry->d_name;
                if (name == "." || name == "..")
                    continue;
                entries.push_back(DirectoryEntry{name, queryPath(joinPath(path, name))});
            }
        }
        catch (...)
        {
            ::closedir(folder);
            throw;
        }
        ::closedir(folder);
        return entries;
    }

    // Права 0777 урезает umask процесса, как у mkdir в оболочке / The 0777 rights are cut by the process umask, as mkdir in a shell
    bool createDirectory(const std::string& path) noexcept
    {
        if (path.empty())
            return false;
        if (::mkdir(path.c_str(), 0777) == 0)
            return true;
        return errno == EEXIST && queryPath(path).directory;
    }

    // Папка (не ссылка на неё) — rmdir, остальное, включая ссылку, — unlink
    // A folder (not a link to one) goes to rmdir, everything else, including a link, goes to unlink
    bool removePath(const std::string& path) noexcept
    {
        FileInfo info = queryPath(path);

        if (path.empty() || (!info.exists && !info.link))
            return false;
        if (info.directory && !info.link)
            return ::rmdir(path.c_str()) == 0;
        return ::unlink(path.c_str()) == 0;
    }

    // rename(2) заменяет существующий файл атомарно / rename(2) replaces an existing file atomically
    bool renamePath(const std::string& from, const std::string& to) noexcept
    {
        return !from.empty() && !to.empty() && std::rename(from.c_str(), to.c_str()) == 0;
    }
}

#endif
