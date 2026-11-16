#if !defined(_WIN32)

#include "gran/utils/internal/file_system_detail.hpp"

#include <cerrno>
#include <cstdlib>
#include <unistd.h>

namespace gran::fs
{
    // getcwd с растущим буфером: длина пути на POSIX не ограничена PATH_MAX надёжно
    // getcwd with a growing buffer: the path length on POSIX is not reliably limited by PATH_MAX
    std::string currentDirectory()
    {
        std::string buffer(256, '\0');

        while (buffer.size() <= 65536)
        {
            if (::getcwd(&buffer[0], buffer.size()) != nullptr)
            {
                buffer.resize(std::char_traits<char>::length(buffer.c_str()));
                return buffer;
            }
            if (errno != ERANGE)
                return std::string();
            buffer.resize(buffer.size() * 2);
        }
        return std::string();
    }

    // TMPDIR, если задана и не пуста, иначе /tmp; «/» в конце снимается / TMPDIR if set and not empty, otherwise /tmp; the trailing "/" is removed
    std::string temporaryDirectory()
    {
        const char* value = std::getenv("TMPDIR");
        std::string path = "/tmp";

        if (value != nullptr && value[0] != '\0')
            path = value;
        while (path.size() > 1 && path.back() == '/')
            path.pop_back();
        return path;
    }

    // На POSIX имя файла — байты, UTF-8 передаётся как есть / On POSIX a file name is bytes, UTF-8 is passed as is
    void openFile(std::ifstream& stream, const std::string& path, std::ios::openmode mode)
    {
        stream.open(path.c_str(), mode | std::ios::in);
    }

    // Запись — тем же путём, что чтение / Writing goes the same way as reading
    void openFile(std::ofstream& stream, const std::string& path, std::ios::openmode mode)
    {
        stream.open(path.c_str(), mode | std::ios::out);
    }
}

#endif
