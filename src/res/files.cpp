#include "gran/res/resources.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace gran::res
{
    // Ресурс копируется в вектор; файл диска читается целиком / A resource is copied into the vector; a disk file is read whole
    bool readFile(const std::filesystem::path& path, std::vector<std::uint8_t>& bytes)
    {
        ResourceView view;
        std::string text = path.u8string();

        if (find(text, view))
        {
            bytes.assign(view.data, view.data + view.size);
            return true;
        }
        if (isResourcePath(text))
            return false;
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return false;
        bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return true;
    }
}
