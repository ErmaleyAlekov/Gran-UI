#ifndef GRAN_RES_RESOURCES_HPP
#define GRAN_RES_RESOURCES_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <istream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gran::res
{
    // Один файл пакета: путь внутри res:/ и байты (статический массив сгенерированного .cpp) / One bundle file: the path inside res:/ and the bytes (a static array of the generated .cpp)
    struct Entry
    {
        const char* path;
        const unsigned char* data;
        std::size_t size;
    };

    // Пакет ресурсов, созданный gran_rcc: имя и записи / A resource bundle created by gran_rcc: the name and the entries
    struct Bundle
    {
        const char* name;
        const Entry* entries;
        std::size_t count;
    };

    // Байты ресурса без копии (живут до конца процесса) / The resource bytes without a copy (they live until the process end)
    struct ResourceView
    {
        const unsigned char* data = nullptr;
        std::size_t size = 0;
    };

    // ---- registry.cpp: реестр пакетов процесса (паттерн Registry) ----
    // ---- registry.cpp: the process bundle registry (the Registry pattern) ----

    /*
    ** Подключить пакет: его файлы становятся доступны как «res:/путь» везде, где движок читает файлы
    ** (loadFile, openWindow, xi:include, картинки, иконки .svg, шрифты, источники JSON/XML). Позже
    ** подключённый пакет перекрывает одинаковые пути. Байты не копируются.
    **
    ** Mount a bundle: its files become available as "res:/path" everywhere the engine reads files
    ** (loadFile, openWindow, xi:include, images, .svg icons, fonts, JSON/XML sources). A bundle mounted later
    ** overrides equal paths. The bytes are not copied.
    */
    void mount(const Bundle& bundle);

    // Снять все пакеты (для тестов) / Unmount all the bundles (for tests)
    void unmountAll() noexcept;

    // Путь вида «res:/…» (или «res:\…» после нормализации путей Windows) / A path like "res:/…" (or "res:\…" after the Windows path normalization)
    bool isResourcePath(std::string_view path) noexcept;

    // Найти ресурс; путь — «res:/a/b.png», «res:/a/./c/../b.png» и с обратными косыми / Find a resource; the path is "res:/a/b.png", "res:/a/./c/../b.png" and with backslashes
    bool find(std::string_view path, ResourceView& view);

    // ---- normalize.cpp ----

    // Путь внутри пакета: без «res:/», косые «/», «.» и «..» разобраны; выход за корень — пусто / The path inside the bundle: without "res:/", the slashes "/", "." and ".." resolved; leaving the root — empty
    std::string normalize(std::string_view path);

    // ---- blob.cpp: пакет одним двоичным блоком (gran_rcc --blob) для обёрток языков ----
    // ---- blob.cpp: a bundle as one binary block (gran_rcc --blob) for the language wrappers ----

    // Подключить блок (байты копируются и живут до конца процесса); испорченный блок — GranException(RESOURCE) / Mount a block (the bytes are copied and live until the process end); a damaged block is GranException(RESOURCE)
    void mountBlob(const void* data, std::size_t size);

    // ---- files.cpp ----

    // Байты файла: ресурс res:/ или файл диска; false — не прочитан / The file bytes: a res:/ resource or a disk file; false — not read
    bool readFile(const std::filesystem::path& path, std::vector<std::uint8_t>& bytes);

    // ---- streams.cpp ----

    // Поток чтения: ресурс res:/ (из памяти) или файл диска; при ошибке поток в состоянии fail / A read stream: a res:/ resource (from memory) or a disk file; on an error the stream is in the fail state
    std::unique_ptr<std::istream> openStream(const std::filesystem::path& path);
}

#endif
