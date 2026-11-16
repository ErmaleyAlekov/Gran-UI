#if defined(_WIN32)

#include "gran/utils/internal/file_system_detail.hpp"

#ifndef NOMINMAX
#   define NOMINMAX
#endif
#include <windows.h>

#include <climits>

namespace gran::fs
{
    // Без MB_ERR_INVALID_CHARS неверные байты UTF-8 становятся U+FFFD, а не ошибкой
    // Without MB_ERR_INVALID_CHARS bad UTF-8 bytes become U+FFFD instead of an error
    std::wstring toWidePath(const std::string& path)
    {
        std::wstring wide;
        int length = 0;

        // Путь длиннее INT_MAX байт не бывает; проверка — до сужения size_t → int
        // A path longer than INT_MAX bytes does not exist; the check goes before narrowing size_t → int
        if (path.empty() || path.size() > static_cast<std::size_t>(INT_MAX))
            return wide;
        length = MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), nullptr, 0);
        if (length <= 0)
            return wide;
        wide.resize(static_cast<std::size_t>(length));
        MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), &wide[0], length);
        return wide;
    }

    // UTF-16 → UTF-8; непарные суррогаты Windows заменяет на U+FFFD / UTF-16 → UTF-8; Windows replaces unpaired surrogates with U+FFFD
    std::string fromWidePath(const wchar_t* text, std::size_t length)
    {
        std::string narrow;
        int size = 0;

        if (text == nullptr || length == 0 || length > static_cast<std::size_t>(INT_MAX))
            return narrow;
        size = WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(length), nullptr, 0, nullptr, nullptr);
        if (size <= 0)
            return narrow;
        narrow.resize(static_cast<std::size_t>(size));
        WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(length), &narrow[0], size, nullptr, nullptr);
        return narrow;
    }
}

namespace gran::fs::internal
{
    /*
    ** FILETIME — сотни наносекунд от 1601-01-01; до эпохи Unix — 11 644 473 600 секунд. Время раньше 1970 года
    ** (испорченные метки) становится нулём, чтобы не уйти в отрицательные числа при умножении.
    **
    ** FILETIME is hundreds of nanoseconds since 1601-01-01; 11 644 473 600 seconds before the Unix epoch. A time before 1970
    ** (broken stamps) becomes zero so as not to go negative in the multiplication.
    */
    FileInfo makeFileInfo(std::uint32_t attributes, std::uint64_t size, std::uint64_t fileTime) noexcept
    {
        const std::uint64_t unixEpoch = 116444736000000000ULL;
        FileInfo info;

        info.exists = true;
        info.directory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        // link решает вызывающий по метке точки повторной обработки (reparseInfo) / link is decided by the caller from the reparse tag (reparseInfo)
        info.regular = !info.directory && (attributes & FILE_ATTRIBUTE_DEVICE) == 0;
        if (info.regular)
            info.size = size;
        // Проверка до умножения: после 2262 года наносекунды не влезают в int64 — время ограничивается
        // The check goes before the multiplication: after 2262 nanoseconds do not fit int64 — the time is clamped
        if (fileTime > unixEpoch && fileTime - unixEpoch <= static_cast<std::uint64_t>(INT64_MAX) / 100ULL)
            info.modified = static_cast<std::int64_t>((fileTime - unixEpoch) * 100ULL);
        else if (fileTime > unixEpoch)
            info.modified = INT64_MAX;
        return info;
    }
}

#endif
