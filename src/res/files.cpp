#include "gran/res/resources.hpp"

#include "gran/utils/file_system.hpp"

#include <fstream>
#include <iterator>

namespace gran::res
{
    // Ресурс копируется в вектор; файл диска читается целиком / A resource is copied into the vector; a disk file is read whole
    bool readFile(const std::string& path, std::vector<std::uint8_t>& bytes)
    {
        ResourceView view;
        std::ifstream file;

        if (find(path, view))
        {
            bytes.assign(view.data, view.data + view.size);
            return true;
        }
        if (isResourcePath(path))
            return false;
        fs::openFile(file, path);
        if (!file)
            return false;
        bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return true;
    }
}
