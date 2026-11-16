#ifndef GRAN_UTILS_INTERNAL_FILE_SYSTEM_DETAIL_HPP
#define GRAN_UTILS_INTERNAL_FILE_SYSTEM_DETAIL_HPP

#include "gran/utils/file_system.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

/*
** Внутренние помощники слоя путей (gran/utils/file_system.hpp): разбор корня пути и перевод сведений ОС
** в FileInfo. Не для программ на движке.
**
** Internal helpers of the path layer (gran/utils/file_system.hpp): parsing the path root and converting the OS
** information into FileInfo. Not for programs on the engine.
*/
namespace gran::fs::internal
{
    // ---- path_root.cpp ----

    // Разделитель ли символ: «/» везде, «\» — только на Windows / Whether the symbol is a separator: "/" everywhere, "\" only on Windows
    bool isPathSeparator(char symbol) noexcept;

    // Длина корня пути: «/» → 1; на Windows «C:» → 2, «C:/» → 3, «//сервер/» — до разделителя после имени сервера
    // The path root length: "/" → 1; on Windows "C:" → 2, "C:/" → 3, "//server/" — up to the separator after the server name
    std::size_t rootLength(const std::string& path) noexcept;

    // Корень пути с разделителями, приведёнными к «/» / The path root with the separators turned into "/"
    std::string rootOf(const std::string& path);

    // Имя корня без разделителя: «C:» или «//сервер» на Windows, иначе «» / The root name without a separator: "C:" or "//server" on Windows, otherwise ""
    std::string rootName(const std::string& path);

#if defined(_WIN32)
    // ---- wide_path_windows.cpp ----

    // Сведения из атрибутов Windows (FILE_ATTRIBUTE_*), размера и FILETIME (сотни нс от 1601 года)
    // Information from the Windows attributes (FILE_ATTRIBUTE_*), the size and FILETIME (hundreds of ns since 1601)
    FileInfo makeFileInfo(std::uint32_t attributes, std::uint64_t size, std::uint64_t fileTime) noexcept;

    // ---- system_path_windows.cpp ----

    // Путь для вызова ОС: длинный (от 240 байт) — абсолютный в форме «\\?\» / A path for an OS call: a long one (from 240 bytes) is absolute in the "\\?\" form
    std::wstring systemPath(const std::string& path);

    // Метка точки повторной обработки — ссылка (символическая или точка соединения) / The reparse point tag is a link (symbolic or a mount point)
    bool isLinkTag(std::uint32_t tag) noexcept;

    // Сведения о цели ссылки path (форма systemPath); висячая — exists == false, link == true
    // The information about the target of the path link (the systemPath form); a dangling one — exists == false, link == true
    FileInfo linkTarget(const std::wstring& path) noexcept;

    // Точка повторной обработки path: ссылка — сведения о цели, иначе own без изменений
    // The path reparse point: a link gives the target information, otherwise own unchanged
    FileInfo reparseInfo(const std::wstring& path, const FileInfo& own) noexcept;
#endif
}

#endif
