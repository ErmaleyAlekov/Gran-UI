#if defined(_WIN32)

#include "gran/utils/internal/file_system_detail.hpp"

#ifndef NOMINMAX
#   define NOMINMAX
#endif
#include <windows.h>

namespace gran::fs::internal
{
    namespace
    {
        // Длиннее — путь уходит в форму «\\?\»: 248 символов — предел CreateDirectoryW, UTF-8 не короче UTF-16
        // Longer — the path goes into the "\\?\" form: 248 symbols is the CreateDirectoryW limit, UTF-8 is not shorter than UTF-16
        constexpr std::size_t LONG_PATH_BYTES = 240;
    }

    /*
    ** Путь для вызова ОС. Короткий — как есть (старые API сами понимают «/», «..» и относительные пути). Длинный —
    ** абсолютный нормализованный с «\» и префиксом «\\?\» («\\?\UNC\» у сетевого): в этой форме Windows не режет путь
    ** на 260 символах, но и не разбирает «/» и «..» — поэтому нормализация делается заранее (найдено ревью Т-57).
    **
    ** A path for an OS call. A short one goes as is (the old APIs understand "/", ".." and relative paths themselves). A long one
    ** becomes absolute normalized with "\" and the "\\?\" prefix ("\\?\UNC\" for a network one): in this form Windows does not cut
    ** the path at 260 symbols, but does not parse "/" and ".." either — so normalization is done beforehand (found by the T-57 review).
    */
    std::wstring systemPath(const std::string& path)
    {
        std::wstring prefix = L"\\\\?\\";
        std::string full;

        if (path.size() < LONG_PATH_BYTES || path.compare(0, 4, "\\\\?\\") == 0)
            return toWidePath(path);
        full = absolutePath(path);
        for (char& symbol : full)
        {
            if (symbol == '/')
                symbol = '\\';
        }
        if (full.size() > 2 && full[0] == '\\' && full[1] == '\\')
        {
            prefix = L"\\\\?\\UNC\\";
            full.erase(0, 2);
        }
        return prefix + toWidePath(full);
    }

    // Ссылки — только символические и точки соединения; заглушки облака (OneDrive) и прочие — обычные объекты
    // Links are only symbolic ones and mount points; cloud placeholders (OneDrive) and others are ordinary objects
    bool isLinkTag(std::uint32_t tag) noexcept
    {
        return tag == IO_REPARSE_TAG_SYMLINK || tag == IO_REPARSE_TAG_MOUNT_POINT;
    }

    // Сведения о цели ссылки: CreateFileW идёт по всей цепочке ссылок; цели нет — висячая ссылка (exists == false)
    // The link target information: CreateFileW follows the whole link chain; no target — a dangling link (exists == false)
    FileInfo linkTarget(const std::wstring& path) noexcept
    {
        BY_HANDLE_FILE_INFORMATION data{};
        FileInfo info;
        HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

        if (file != INVALID_HANDLE_VALUE)
        {
            if (GetFileInformationByHandle(file, &data))
                info = makeFileInfo(data.dwFileAttributes,
                    (static_cast<std::uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow,
                    (static_cast<std::uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime);
            CloseHandle(file);
        }
        info.link = true;
        return info;
    }

    // Точка повторной обработки: метка читается с самой точки (без перехода); ссылка — сведения о цели, иначе — own
    // A reparse point: the tag is read from the point itself (without following); a link gives the target information, otherwise own
    FileInfo reparseInfo(const std::wstring& path, const FileInfo& own) noexcept
    {
        FILE_ATTRIBUTE_TAG_INFO tag{};
        BOOL read = FALSE;
        HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

        if (file == INVALID_HANDLE_VALUE)
            return own;
        read = GetFileInformationByHandleEx(file, FileAttributeTagInfo, &tag, sizeof(tag));
        CloseHandle(file);
        if (read && isLinkTag(tag.ReparseTag))
            return linkTarget(path);
        return own;
    }
}

#endif
