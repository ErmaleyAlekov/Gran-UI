#include "gran/res/manifest.hpp"
#include "gran/res/resources.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cstdint>

namespace gran::res
{
    namespace
    {
        // Число little-endian из bytes байт / A little-endian number of bytes bytes
        void putNumber(std::string& out, std::uint64_t value, std::size_t bytes)
        {
            for (std::size_t index = 0; index < bytes; ++index)
                out += static_cast<char>((value >> (8 * index)) & 0xFFu);
        }

        // Путь для make: косые «/», пробелы экранируются / A path for make: "/" slashes, spaces are escaped
        std::string makePath(const std::filesystem::path& path)
        {
            std::string text = path.generic_u8string();
            std::string result;

            for (char symbol : text)
            {
                if (symbol == ' ')
                    result += '\\';
                result += symbol;
            }
            return result;
        }
    }

    // «GRANRES1», число записей, затем путь и байты каждой (формат читает mountBlob) / "GRANRES1", the entry count, then the path and the bytes of each (mountBlob reads the format)
    std::string generateBlob(const Manifest& manifest)
    {
        std::string out = "GRANRES1";
        std::vector<std::uint8_t> bytes;

        putNumber(out, manifest.files.size(), 4);
        for (const ManifestFile& file : manifest.files)
        {
            if (!readFile(file.source, bytes))
                throw GranException(ErrorCode::RESOURCE, i18n::format(i18n::Message::RES_MISSING_FILE,
                    {file.source.u8string()}));
            putNumber(out, file.alias.size(), 4);
            out += file.alias;
            putNumber(out, bytes.size(), 8);
            out.append(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        }
        return out;
    }

    // Цель зависит от манифеста и всех файлов пакета: правка картинки пересоберёт .cpp / The target depends on the manifest and all the bundle files: editing an image rebuilds the .cpp
    std::string generateDepfile(const Manifest& manifest, const std::string& target)
    {
        std::string out = makePath(std::filesystem::u8path(target)) + ": " + makePath(manifest.location);

        for (const ManifestFile& file : manifest.files)
            out += " \\\n  " + makePath(file.source);
        out += '\n';
        return out;
    }
}
