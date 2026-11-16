#if defined(_WIN32)

#include "gran/utils/internal/file_system_detail.hpp"

#ifndef NOMINMAX
#   define NOMINMAX
#endif
#include <windows.h>

#include <cwchar>

namespace gran::fs
{
    /*
    ** GetFileAttributesExW не идёт по ссылке, поэтому у точки повторной обработки сведения берутся у цели
    ** (reparseInfo): висячая ссылка — пути нет, как у stat на POSIX (раньше она выглядела обычным файлом — ревью Т-57).
    **
    ** GetFileAttributesExW does not follow a link, so for a reparse point the information comes from the target
    ** (reparseInfo): a dangling link means no path, as stat on POSIX (earlier it looked like a regular file — the T-57 review).
    */
    FileInfo queryPath(const std::string& path) noexcept
    {
        WIN32_FILE_ATTRIBUTE_DATA data{};
        FileInfo info;

        try
        {
            std::wstring wide = internal::systemPath(path);
            if (wide.empty() || !GetFileAttributesExW(wide.c_str(), GetFileExInfoStandard, &data))
                return FileInfo{};
            info = internal::makeFileInfo(data.dwFileAttributes,
                (static_cast<std::uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow,
                (static_cast<std::uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime);
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                return internal::reparseInfo(wide, info);
        }
        catch (...)
        {
            // Нехватка памяти при переводе пути — путь считается недоступным
            // Out of memory while converting the path — the path is treated as unavailable
            return FileInfo{};
        }
        return info;
    }

    // FindFirstFileExW по маске «папка\*»; у ссылок (метка в dwReserved0) сведения — о цели через queryPath;
    // дескриптор поиска закрывается и при исключении push_back
    // FindFirstFileExW by the "folder\*" mask; for links (the tag in dwReserved0) the information is about the target through queryPath;
    // the search handle is closed on a push_back exception as well
    std::vector<DirectoryEntry> listDirectory(const std::string& path)
    {
        std::vector<DirectoryEntry> entries;
        WIN32_FIND_DATAW data{};
        HANDLE search = FindFirstFileExW(internal::systemPath(joinPath(path, "*")).c_str(), FindExInfoBasic, &data,
            FindExSearchNameMatch, nullptr, 0);

        if (search == INVALID_HANDLE_VALUE)
            return entries;
        try
        {
            do
            {
                DirectoryEntry entry{fromWidePath(data.cFileName, wcslen(data.cFileName)), internal::makeFileInfo(
                    data.dwFileAttributes, (static_cast<std::uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow,
                    (static_cast<std::uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime)};
                if (entry.name == "." || entry.name == ".." || entry.name.empty())
                    continue;
                if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 && internal::isLinkTag(data.dwReserved0))
                    entry.info = queryPath(joinPath(path, entry.name));
                entries.push_back(entry);
            } while (FindNextFileW(search, &data));
        }
        catch (...)
        {
            FindClose(search);
            throw;
        }
        FindClose(search);
        return entries;
    }

    // Уже существующая папка — тоже успех / An already existing folder is a success too
    bool createDirectory(const std::string& path) noexcept
    {
        try
        {
            std::wstring wide = internal::systemPath(path);
            if (!wide.empty() && CreateDirectoryW(wide.c_str(), nullptr))
                return true;
        }
        catch (...)
        {
            return false;
        }
        return queryPath(path).directory;
    }

    /*
    ** Решение — по атрибутам самого пути (без перехода по ссылке): папка и ссылка на папку (в том числе висячая)
    ** удаляются RemoveDirectoryW — у ссылки удаляется она сама, а не цель; файл и ссылка на файл — DeleteFileW.
    **
    ** The decision goes by the attributes of the path itself (without following a link): a folder and a link to a folder
    ** (a dangling one too) are deleted by RemoveDirectoryW — for a link the link itself goes, not the target; a file and a link
    ** to a file go to DeleteFileW.
    */
    bool removePath(const std::string& path) noexcept
    {
        try
        {
            std::wstring wide = internal::systemPath(path);
            DWORD attributes = GetFileAttributesW(wide.c_str());
            if (wide.empty() || attributes == INVALID_FILE_ATTRIBUTES)
                return false;
            if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                return RemoveDirectoryW(wide.c_str()) != 0;
            return DeleteFileW(wide.c_str()) != 0;
        }
        catch (...)
        {
            return false;
        }
    }

    // MOVEFILE_REPLACE_EXISTING — как rename на POSIX; COPY_ALLOWED — перенос между дисками
    // MOVEFILE_REPLACE_EXISTING is like rename on POSIX; COPY_ALLOWED is a move between disks
    bool renamePath(const std::string& from, const std::string& to) noexcept
    {
        try
        {
            std::wstring source = internal::systemPath(from);
            std::wstring target = internal::systemPath(to);
            return MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;
        }
        catch (...)
        {
            return false;
        }
    }
}

#endif
