#ifndef GRAN_TEXT_FONT_LIBRARY_HPP
#define GRAN_TEXT_FONT_LIBRARY_HPP

#include "gran/text/font_face.hpp"

#include <memory>
#include <string>
#include <vector>

namespace gran::text
{
    // Кандидат: начертание в файле, его признаки и (после первой загрузки) объект
    // A candidate: a face in a file, its traits and (after the first load) the object
    struct FontCandidate
    {
        std::string family;
        std::string path;
        std::uint32_t index = 0;
        int weight = 400;
        bool italic = false;
        bool failed = false;
        std::shared_ptr<const FontFace> face;
    };

    /*
    ** Библиотека шрифтов окна: зарегистрированные (<Font>) и системные начертания.
    ** Системные каталоги сканируются лениво, один раз и только по таблицам name и OS/2
    ** (файл целиком загружается, лишь когда начертание действительно выбрано).
    ** Не потокобезопасна: используется потоком интерфейса.
    **
    ** The window font library: registered (<Font>) and system faces.
    ** The system directories are scanned lazily, once and only by the name and OS/2 tables
    ** (a file is loaded entirely only when the face is actually chosen).
    ** Not thread-safe: used by the interface thread.
    */
    class FontLibrary
    {
    public:
        // ---- font_library.cpp ----

        // Все начертания файла (TTC — несколько) / All faces of a file (a TTC has several)
        void addFile(const std::string& path);

        /*
        ** Лучшее начертание по списку семейств через запятую ("Inter, Segoe UI, sans-serif"):
        ** первое найденное семейство, в нём — ближайшая насыщенность и совпадение курсива.
        ** Ничего не найдено — системное начертание без засечек.
        **
        ** The best face by a comma separated family list ("Inter, Segoe UI, sans-serif"):
        ** the first family found, in it — the nearest weight and the matching italic.
        ** Nothing found — the system sans-serif face.
        */
        std::shared_ptr<const FontFace> match(const std::string& families, int weight, bool italic);

        // Начертание с глифом символа для замены (эмодзи, CJK, символы) / A face with the character glyph for fallback
        std::shared_ptr<const FontFace> fallback(char32_t symbol, int weight, bool italic);

    private:
        // Открытый файл шрифта: путь и слабая ссылка на отображение / An opened font file: the path and a weak reference to the mapping
        struct OpenedFile
        {
            std::string path;
            std::weak_ptr<const MappedFile> file;
        };

        // ---- font_files.cpp ----

        // Отображение файла, общее для всех его начертаний / The file mapping shared by all its faces
        std::shared_ptr<const MappedFile> openFile(const std::string& path);

        // Объект начертания кандидата (загрузка при первом обращении) / The candidate face object (loading on first access)
        std::shared_ptr<const FontFace> loadCandidate(FontCandidate& candidate);

        // ---- font_matching.cpp ----

        // Лучший кандидат одного семейства или nullptr / The best candidate of one family or nullptr
        FontCandidate* bestInFamily(const std::string& family, int weight, bool italic);

        // Системные каталоги — один раз / The system directories — once
        void ensureScanned();

        // Известные файлы семейства — один раз на семейство (быстрее полного сканирования)
        // The known files of the family — once per family (faster than a full scan)
        bool probeKnown(const std::string& family);

        std::vector<FontCandidate> _candidates;
        std::vector<std::string> _probed;
        std::vector<OpenedFile> _files;
        bool _scanned = false;
    };

    // ---- font_scan.cpp ----

    // Каталоги системных шрифтов текущей ОС / The system font directories of the current OS
    std::vector<std::string> systemFontDirectories();

    // Признаки начертаний файла без полной загрузки (name, OS/2) / The file face traits without a full load (name, OS/2)
    void scanFontFile(const std::string& path, std::vector<FontCandidate>& candidates);

    // ---- font_known.cpp ----

    // Обобщённое семейство ОС: sans-serif, serif, monospace, system-ui → список имён
    // An OS generic family: sans-serif, serif, monospace, system-ui → a name list
    std::vector<std::string> genericFamilies(const std::string& generic);

    // Известные файлы распространённого семейства в системных каталогах (пусто — не известны)
    // The known files of a common family in the system directories (empty — not known)
    std::vector<std::string> knownFontFiles(const std::string& family);

    // Сравнение имён семейств без учёта регистра / A family name comparison ignoring case
    bool sameFamilyName(const std::string& first, const std::string& second) noexcept;
}

#endif
