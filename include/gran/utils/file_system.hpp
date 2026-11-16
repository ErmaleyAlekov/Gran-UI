#ifndef GRAN_UTILS_FILE_SYSTEM_HPP
#define GRAN_UTILS_FILE_SYSTEM_HPP

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

/*
** Свой слой путей и файловой системы вместо std::filesystem.
** Зачем: на Windows движок открывал часть файлов через ANSI (CreateFileA, path::string() в MinGW), и путь
** с символами вне кодовой страницы ANSI (кириллица при английской локали, китайские имена…) не открывался.
** Здесь путь — всегда UTF-8, а на Windows он переводится в UTF-16 для широких API (…W). Заодно в коде Грани
** нет типов стандартной библиотеки с неустойчивым ABI (std::filesystem в GCC 8 экспериментальный). «Берёста» по-прежнему использует std::filesystem, поэтому архивы выпуска Linux собираются
** отдельно под GCC 8 и под GCC 10+ (docs/building.md).
**
** Правила:
**  - путь — строка UTF-8 на всех ОС; на Windows функции переводят её в UTF-16 и зовут широкие API (…W);
**  - разделитель в результатах склейки, нормализации и вызовов ОС — «/»; на Windows на входе понимается и «\»
**    (fileName и parentPath режут входную строку как есть, разделители в ней не меняются);
**  - функции не бросают исключений файловой системы: ошибка — false, пустой результат или FileInfo.exists == false;
**  - разбор путей (joinPath, parentPath, normalizePath…) чисто строковый, к диску не обращается, как lexically_*.
**
** Own path and file system layer instead of std::filesystem.
** Why: on Windows the engine opened some files through ANSI (CreateFileA, path::string() in MinGW), and a path
** with symbols outside the ANSI code page (Cyrillic under an English locale, Chinese names…) did not open.
** Here a path is always UTF-8, and on Windows it is converted to UTF-16 for the wide APIs (…W). As a bonus the Gran
** code has no standard library types with an unstable ABI (std::filesystem is experimental in GCC 8). Beresta still uses std::filesystem, so the Linux release archives are built separately
** for GCC 8 and for GCC 10+ (docs/building.en.md).
**
** Rules:
**  - a path is a UTF-8 string on every OS; on Windows the functions convert it to UTF-16 and call the wide APIs (…W);
**  - the separator in the results of joining, normalization and the OS calls is "/"; on Windows "\" is understood on input
**    as well (fileName and parentPath cut the input string as is, its separators are not changed);
**  - the functions throw no file system exceptions: an error is false, an empty result or FileInfo.exists == false;
**  - path parsing (joinPath, parentPath, normalizePath…) is purely textual and does not touch the disk, like lexically_*.
*/
namespace gran::fs
{
    // Сведения о файле или папке / Information about a file or a folder
    struct FileInfo
    {
        // Путь существует / The path exists
        bool exists = false;
        // Это папка (иначе файл или другой объект) / It is a folder (otherwise a file or another object)
        bool directory = false;
        // Это обычный файл / It is a regular file
        bool regular = false;
        // Сам путь — ссылка (символическая или точка повторной обработки Windows) / The path itself is a link (symbolic or a Windows reparse point)
        bool link = false;
        // Размер файла в байтах (у папки 0) / The file size in bytes (0 for a folder)
        std::uint64_t size = 0;
        // Время последней записи: наносекунды от 1970-01-01 UTC; сравнивается только между собой
        // The last write time: nanoseconds since 1970-01-01 UTC; compared only with each other
        std::int64_t modified = 0;
    };

    // Элемент папки (без «.» и «..») / A folder entry (without "." and "..")
    struct DirectoryEntry
    {
        // Имя без пути / The name without the path
        std::string name;
        // Сведения об элементе / The entry information
        FileInfo info;
    };

    // ---- path_parts.cpp: части пути (только строки) / path parts (strings only) ----

    // Последний элемент пути: «a/b.txt» → «b.txt», «a/» → «» / The last path element: "a/b.txt" → "b.txt", "a/" → ""
    std::string fileName(const std::string& path);

    // Расширение имени с точкой: «a/b.tar.gz» → «.gz»; у «.profile», «.», «..» — пустое
    // The name extension with the dot: "a/b.tar.gz" → ".gz"; empty for ".profile", ".", ".."
    std::string fileExtension(const std::string& path);

    // Имя без расширения: «a/b.txt» → «b» / The name without the extension: "a/b.txt" → "b"
    std::string fileStem(const std::string& path);

    // Путь без последнего элемента: «a/b» → «a», «/a» → «/», «C:/a» → «C:/», «a» → «»
    // The path without the last element: "a/b" → "a", "/a" → "/", "C:/a" → "C:/", "a" → ""
    std::string parentPath(const std::string& path);

    // Абсолютный ли путь: «/…» на POSIX; «C:/…», «C:\…» и «//сервер/…» на Windows
    // Whether the path is absolute: "/…" on POSIX; "C:/…", "C:\…" and "//server/…" on Windows
    bool isAbsolutePath(const std::string& path);

    // ---- path_join.cpp: склейка и нормализация (только строки) / joining and normalization (strings only) ----

    // base + «/» + tail; абсолютный tail заменяет base, пустая часть не добавляет разделитель
    // base + "/" + tail; an absolute tail replaces base, an empty part adds no separator
    std::string joinPath(const std::string& base, const std::string& tail);

    // Без «.», с разрешёнными «..», одиночными «/» и без «/» в конце; «a/..» → «.», «» → «»
    // Without ".", with resolved "..", single "/" and no trailing "/"; "a/.." → ".", "" → ""
    std::string normalizePath(const std::string& path);

    // Абсолютный нормализованный путь относительно текущей папки процесса
    // An absolute normalized path relative to the current process folder
    std::string absolutePath(const std::string& path);

    // ---- file_query.cpp: вопросы к диску / questions to the disk ----

    // Существует ли путь / Whether the path exists
    bool pathExists(const std::string& path);

    // Обычный ли это файл / Whether it is a regular file
    bool isRegularFile(const std::string& path);

    // Папка ли это / Whether it is a folder
    bool isDirectory(const std::string& path);

    // Создание папки со всеми недостающими родителями; true — папка есть (в том числе была)
    // Creating a folder with all missing parents; true — the folder exists (including it existed before)
    bool createDirectories(const std::string& path);

    // ---- file_list.cpp: обход папок / walking folders ----

    // Обычные файлы папки, рекурсивно или нет, — пути относительно root через «/», по алфавиту;
    // в папки-ссылки (символические ссылки, точки повторной обработки) не спускается, чтобы не зациклиться
    // The regular files of the folder, recursive or not — paths relative to root with "/", sorted;
    // it does not descend into link folders (symbolic links, reparse points) to avoid loops
    std::vector<std::string> listFiles(const std::string& root, bool recursive);

    // ---- file_system_windows.cpp / file_system_posix.cpp: вызовы ОС / the OS calls ----

    // Сведения о пути; у ссылки link == true, остальное — о цели (на Windows — флаги самой ссылки)
    // Information about the path; a link has link == true, the rest is about the target (on Windows the link's own flags)
    FileInfo queryPath(const std::string& path) noexcept;

    // Элементы одной папки; папка не открылась — пустой список
    // The entries of one folder; the folder did not open — an empty list
    std::vector<DirectoryEntry> listDirectory(const std::string& path);

    // Создание одной папки; true — создана или уже была / Creating one folder; true — created or existed already
    bool createDirectory(const std::string& path) noexcept;

    // Удаление файла или пустой папки; true — удалено / Deleting a file or an empty folder; true — deleted
    bool removePath(const std::string& path) noexcept;

    // Переименование с заменой существующего файла; true — сделано / Renaming with replacing an existing file; true — done
    bool renamePath(const std::string& from, const std::string& to) noexcept;

    // ---- file_places_windows.cpp / file_places_posix.cpp: места и потоки / places and streams ----

    // Текущая папка процесса; ошибка — «» / The current process folder; an error — ""
    std::string currentDirectory();

    // Папка временных файлов (TEMP / TMPDIR, иначе /tmp) / The temporary files folder (TEMP / TMPDIR, otherwise /tmp)
    std::string temporaryDirectory();

    // Открыть поток чтения по пути UTF-8 (std::ios::in добавляется); проверка — fail() потока
    // Open a read stream by a UTF-8 path (std::ios::in is added); check the stream fail()
    void openFile(std::ifstream& stream, const std::string& path, std::ios::openmode mode = std::ios::binary);

    // Открыть поток записи по пути UTF-8 (std::ios::out добавляется); проверка — fail() потока
    // Open a write stream by a UTF-8 path (std::ios::out is added); check the stream fail()
    void openFile(std::ofstream& stream, const std::string& path,
        std::ios::openmode mode = std::ios::binary | std::ios::trunc);

#if defined(_WIN32)
    // UTF-8 → UTF-16 для широких API Windows (неверные байты — U+FFFD) / UTF-8 → UTF-16 for the Windows wide APIs (bad bytes — U+FFFD)
    std::wstring toWidePath(const std::string& path);

    // UTF-16 → UTF-8 (length символов; непарные суррогаты — U+FFFD) / UTF-16 → UTF-8 (length symbols; unpaired surrogates — U+FFFD)
    std::string fromWidePath(const wchar_t* text, std::size_t length);
#endif
}

#endif
