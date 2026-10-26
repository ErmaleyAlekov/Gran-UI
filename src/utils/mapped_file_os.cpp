#include "gran/utils/mapped_file.hpp"

#if defined(_WIN32)
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#else
#   include <fcntl.h>
#   include <sys/mman.h>
#   include <sys/stat.h>
#   include <unistd.h>
#endif

namespace gran
{
#if defined(_WIN32)
    /*
    ** Windows: файл → объект отображения → вид. Дескриптор файла после создания объекта
    ** не нужен (объект держит файл сам), дескриптор объекта закрывается в unmap.
    ** Путь — в кодовой странице ANSI, как у std::filesystem::path::string() в MinGW.
    **
    ** Windows: the file → the mapping object → the view. The file handle is not needed after
    ** creating the object (the object holds the file itself), the object handle is closed in unmap.
    ** The path is in the ANSI code page, as std::filesystem::path::string() gives in MinGW.
    */
    bool MappedFile::map(const std::string& path)
    {
        HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        LARGE_INTEGER length{};

        if (file == INVALID_HANDLE_VALUE)
            return false;
        // Пустой файл отобразить нельзя: CreateFileMapping отвечает ошибкой
        // An empty file cannot be mapped: CreateFileMapping fails
        if (GetFileSizeEx(file, &length) && length.QuadPart > 0)
            _mapping = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        CloseHandle(file);
        if (_mapping == nullptr)
            return false;
        _data = static_cast<const std::uint8_t*>(MapViewOfFile(_mapping, FILE_MAP_READ, 0, 0, 0));
        if (_data == nullptr)
        {
            unmap();
            return false;
        }
        _size = static_cast<std::size_t>(length.QuadPart);
        return true;
    }

    // Вид, затем объект отображения / The view, then the mapping object
    void MappedFile::unmap() noexcept
    {
        if (_data != nullptr)
            UnmapViewOfFile(_data);
        if (_mapping != nullptr)
            CloseHandle(_mapping);
        _data = nullptr;
        _mapping = nullptr;
        _size = 0;
    }
#else
    // POSIX: mmap только для чтения; дескриптор файла после mmap закрывается / POSIX: a read-only mmap; the file descriptor is closed after mmap
    bool MappedFile::map(const std::string& path)
    {
        int file = ::open(path.c_str(), O_RDONLY);
        struct stat info{};
        void* view = MAP_FAILED;

        if (file < 0)
            return false;
        if (fstat(file, &info) == 0 && info.st_size > 0)
            view = mmap(nullptr, static_cast<std::size_t>(info.st_size), PROT_READ, MAP_PRIVATE, file, 0);
        close(file);
        if (view == MAP_FAILED)
            return false;
        _data = static_cast<const std::uint8_t*>(view);
        _size = static_cast<std::size_t>(info.st_size);
        return true;
    }

    // munmap всего вида / munmap of the whole view
    void MappedFile::unmap() noexcept
    {
        if (_data != nullptr)
            munmap(const_cast<std::uint8_t*>(_data), _size);
        _data = nullptr;
        _mapping = nullptr;
        _size = 0;
    }
#endif
}
