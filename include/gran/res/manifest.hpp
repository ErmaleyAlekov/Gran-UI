#ifndef GRAN_RES_MANIFEST_HPP
#define GRAN_RES_MANIFEST_HPP

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace gran::res
{
    // Файл пакета: где лежит на диске и путь внутри res:/ / A bundle file: where it lies on disk and the path inside res:/
    struct ManifestFile
    {
        std::filesystem::path source;
        std::string alias;
    };

    // Разобранный манифест: имя пакета и файлы по возрастанию alias / A parsed manifest: the bundle name and the files by ascending alias
    struct Manifest
    {
        std::string name;
        std::filesystem::path location;
        std::vector<ManifestFile> files;
    };

    // ---- manifest_read.cpp ----

    /*
    ** Манифест <Bundle> со схемой gran-resources.xsd: <File> и <Folder> раскрываются в список файлов,
    ** пути — относительно манифеста. Ошибка схемы — MarkupException; нет файла или папки, повтор alias —
    ** GranException(RESOURCE).
    **
    ** A <Bundle> manifest with the gran-resources.xsd schema: <File> and <Folder> expand into a file list,
    ** the paths are relative to the manifest. A schema error is MarkupException; a missing file or folder, a repeated alias —
    ** GranException(RESOURCE).
    */
    Manifest readManifest(const std::filesystem::path& path);

    // ---- manifest_folder.cpp ----

    // Имя файла подходит под шаблон («*» — любые символы, «?» — один) / The file name matches the pattern ("*" — any characters, "?" — one)
    bool matchPattern(std::string_view pattern, std::string_view name) noexcept;

    // Файлы папки по шаблонам через пробел (с подпапками при recursive) в files / The folder files by space-separated patterns (with subfolders when recursive) into files
    void expandFolder(const std::filesystem::path& folder, const std::string& alias, const std::string& patterns,
        bool recursive, std::vector<ManifestFile>& files);

    // Файлы по возрастанию alias (одинаковый результат на любой ОС); повтор alias — GranException(RESOURCE)
    // The files by ascending alias (the same result on any OS); a repeated alias is GranException(RESOURCE)
    void sortFiles(std::vector<ManifestFile>& files);

    // ---- generate_source.cpp ----

    // Текст .cpp: массивы байтов и функция granResources_<имя>() / The .cpp text: the byte arrays and the granResources_<name>() function
    std::string generateSource(const Manifest& manifest);

    // ---- generate_blob.cpp ----

    // Двоичный блок «GRANRES1» для mountBlob и обёрток / The "GRANRES1" binary block for mountBlob and the wrappers
    std::string generateBlob(const Manifest& manifest);

    // Файл зависимостей make: «цель: манифест файлы…» / A make dependency file: "target: manifest files…"
    std::string generateDepfile(const Manifest& manifest, const std::string& target);
}

#endif
