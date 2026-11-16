#if defined(_WIN32)

#include "gran/utils/internal/file_system_detail.hpp"

#ifndef NOMINMAX
#   define NOMINMAX
#endif
#include <windows.h>

namespace gran::fs
{
    namespace
    {
        // Путь Windows → путь движка: «\» → «/», без разделителя в конце (кроме корня «C:/»)
        // A Windows path → an engine path: "\" → "/", no trailing separator (except the root "C:/")
        std::string enginePath(const std::wstring& wide, std::size_t length)
        {
            std::string path = fromWidePath(wide.c_str(), length);

            for (char& symbol : path)
            {
                if (symbol == '\\')
                    symbol = '/';
            }
            while (path.size() > internal::rootLength(path) && path.back() == '/')
                path.pop_back();
            return path;
        }
    }

    // Два вызова: узнать длину, затем прочитать; папка могла смениться между ними — тогда длина не совпадёт
    // Two calls: learn the length, then read; the folder might change between them — then the length does not match
    std::string currentDirectory()
    {
        DWORD length = GetCurrentDirectoryW(0, nullptr);
        std::wstring wide;

        if (length == 0)
            return std::string();
        wide.resize(length);
        length = GetCurrentDirectoryW(static_cast<DWORD>(wide.size()), &wide[0]);
        if (length == 0 || length >= wide.size())
            return std::string();
        return enginePath(wide, length);
    }

    // GetTempPathW смотрит TMP, TEMP, USERPROFILE, затем папку Windows / GetTempPathW looks at TMP, TEMP, USERPROFILE, then the Windows folder
    std::string temporaryDirectory()
    {
        std::wstring wide(MAX_PATH + 1, L'\0');
        DWORD length = GetTempPathW(static_cast<DWORD>(wide.size()), &wide[0]);

        if (length == 0 || length >= wide.size())
            return std::string();
        return enginePath(wide, length);
    }

    // open с wchar_t* есть у MSVC и у libstdc++ MinGW-w64 — путь UTF-8 не теряется в ANSI
    // open with wchar_t* exists in MSVC and in the MinGW-w64 libstdc++ — the UTF-8 path is not lost in ANSI
    void openFile(std::ifstream& stream, const std::string& path, std::ios::openmode mode)
    {
        stream.open(internal::systemPath(path).c_str(), mode | std::ios::in);
    }

    // Запись — тем же путём, что чтение / Writing goes the same way as reading
    void openFile(std::ofstream& stream, const std::string& path, std::ios::openmode mode)
    {
        stream.open(internal::systemPath(path).c_str(), mode | std::ios::out);
    }
}

#endif
