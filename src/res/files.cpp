#include "gran/res/resources.hpp"

#if defined(_WIN32)
#include "gran/utils/internal/file_system_detail.hpp"
#endif

#include <cstdio>
#include <memory>

namespace gran::res
{
    static std::FILE* openBinary(const std::string& path)
    {
#if defined(_WIN32)
        return _wfopen(fs::internal::systemPath(path).c_str(), L"rb");
#else
        return std::fopen(path.c_str(), "rb");
#endif
    }

    // Ресурс копируется в вектор; файл диска читается целиком / A resource is copied into the vector; a disk file is read whole
    bool readFile(const std::string& path, std::vector<std::uint8_t>& bytes)
    {
        ResourceView view;

        if (find(path, view))
        {
            bytes.assign(view.data, view.data + view.size);
            return true;
        }
        if (isResourcePath(path))
            return false;
        std::unique_ptr<std::FILE, decltype(&std::fclose)> file(openBinary(path), &std::fclose);
        if (!file)
            return false;
        std::vector<std::uint8_t> loaded;
        std::uint8_t buffer[8192];
        while (const auto count = std::fread(buffer, 1, sizeof(buffer), file.get()))
            loaded.insert(loaded.end(), buffer, buffer + count);
        if (std::ferror(file.get()) || !std::feof(file.get()))
            return false;
        bytes.swap(loaded);
        return true;
    }
}
