#include "gran/text/font_library.hpp"
#include "gran/text/internal/font_parts.hpp"

#include <cstdlib>
#include <fstream>

namespace gran::text
{
    namespace
    {
        // Предел байтов таблицы при сканировании (защита от испорченного файла) / The table byte limit while scanning (a guard against a damaged file)
        constexpr std::uint32_t SCAN_LIMIT = 1u << 20;

        // Чтение участка файла; false — файл короче / Reading a file part; false — the file is shorter
        bool readBytes(std::ifstream& file, std::uint32_t offset, std::uint32_t length, std::vector<std::uint8_t>& bytes)
        {
            bytes.assign(length, 0);
            file.clear();
            file.seekg(offset);
            file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(length));
            return static_cast<std::uint32_t>(file.gcount()) == length;
        }

        // Признаки из таблиц name и OS/2 по каталогу / The traits from the name and OS/2 tables by the directory
        void readTraits(std::ifstream& file, const FontReader& directory, std::uint16_t count, FontCandidate& candidate)
        {
            std::vector<std::uint8_t> table;

            for (std::uint16_t entry = 0; entry < count; ++entry)
            {
                std::uint32_t tag = directory.u32(entry * 16u);
                std::uint32_t length = directory.u32(entry * 16u + 12);
                bool wanted = tag == fontTag("name") || tag == fontTag("OS/2");
                if (!wanted || length > SCAN_LIMIT || !readBytes(file, directory.u32(entry * 16u + 8), length, table))
                    continue;
                FontReader reader(table.data(), table.size());
                if (tag == fontTag("name"))
                    candidate.family = readFaceName(reader, TableSpan{0, length}, false);
                else if (length >= 64)
                {
                    candidate.weight = reader.u16(4);
                    candidate.italic = (reader.u16(62) & 1) != 0;
                }
            }
        }

        /*
        ** Одно начертание: каталог таблиц, затем только name и OS/2. Контуры CFF ('OTTO')
        ** пропускаются — движок их пока не рисует.
        **
        ** One face: the table directory, then only name and OS/2. CFF outlines ('OTTO')
        ** are skipped — the engine does not draw them yet.
        */
        void scanFace(std::ifstream& file, const std::string& path, std::uint32_t index, std::uint32_t offset,
            std::vector<FontCandidate>& candidates)
        {
            std::vector<std::uint8_t> head;
            FontCandidate candidate{std::string(), path, index, 400, false, false, nullptr};

            if (!readBytes(file, offset, 12, head) || FontReader(head.data(), head.size()).u32(0) == fontTag("OTTO"))
                return;
            std::uint16_t count = FontReader(head.data(), head.size()).u16(4);
            if (!readBytes(file, offset + 12, static_cast<std::uint32_t>(count) * 16, head))
                return;
            readTraits(file, FontReader(head.data(), head.size()), count, candidate);
            if (!candidate.family.empty())
                candidates.push_back(candidate);
        }
    }

    // TTC — до 64 начертаний; TTF — одно / A TTC — up to 64 faces; a TTF — one
    void scanFontFile(const std::string& path, std::vector<FontCandidate>& candidates)
    {
        std::ifstream file(path, std::ios::binary);
        std::vector<std::uint8_t> header;

        if (!file || !readBytes(file, 0, 12, header))
            return;
        FontReader reader(header.data(), header.size());
        if (reader.u32(0) != fontTag("ttcf"))
        {
            scanFace(file, path, 0, 0, candidates);
            return;
        }
        std::uint32_t count = reader.u32(8);
        if (count > 64 || !readBytes(file, 12, count * 4, header))
            return;
        std::vector<std::uint8_t> offsets = header;
        for (std::uint32_t index = 0; index < count; ++index)
            scanFace(file, path, index, FontReader(offsets.data(), offsets.size()).u32(index * 4), candidates);
    }

    // Каталоги системы и пользователя для текущей ОС / The system and user directories of the current OS
    std::vector<std::string> systemFontDirectories()
    {
        std::vector<std::string> directories;
        const char* home = std::getenv("HOME");
#if defined(_WIN32)
        const char* windows = std::getenv("WINDIR");
        const char* local = std::getenv("LOCALAPPDATA");
        if (windows != nullptr)
            directories.push_back(std::string(windows) + "\\Fonts");
        if (local != nullptr)
            directories.push_back(std::string(local) + "\\Microsoft\\Windows\\Fonts");
#elif defined(__APPLE__)
        directories = {"/System/Library/Fonts", "/Library/Fonts"};
        if (home != nullptr)
            directories.push_back(std::string(home) + "/Library/Fonts");
#else
        directories = {"/usr/share/fonts", "/usr/local/share/fonts"};
        if (home != nullptr)
            directories.push_back(std::string(home) + "/.local/share/fonts");
#endif
        (void)home;
        return directories;
    }
}
