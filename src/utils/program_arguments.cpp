#include "gran/utils/program_arguments.hpp"
#include "gran/utils/file_system.hpp"

#if defined(_WIN32)
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#   include <shellapi.h>

#   include <cwchar>
#endif

namespace gran
{
#if defined(_WIN32)
    /*
    ** Windows: широкая строка процесса → UTF-8. Не разобралась (нет памяти) — запасной путь через argv из main,
    ** чтобы программа хотя бы получила ASCII-аргументы. Массив CommandLineToArgvW освобождается LocalFree и при
    ** исключении push_back.
    **
    ** Windows: the process wide line → UTF-8. Not parsed (no memory) — the fallback through argv from main,
    ** so that the program gets at least the ASCII arguments. The CommandLineToArgvW array is freed by LocalFree on
    ** a push_back exception as well.
    */
    std::vector<std::string> programArguments(int argc, char* argv[])
    {
        std::vector<std::string> arguments;
        int count = 0;
        LPWSTR* wide = CommandLineToArgvW(GetCommandLineW(), &count);

        if (wide == nullptr)
            return std::vector<std::string>(argv, argv + argc);
        try
        {
            for (int index = 0; index < count; ++index)
                arguments.push_back(fs::fromWidePath(wide[index], std::wcslen(wide[index])));
        }
        catch (...)
        {
            LocalFree(wide);
            throw;
        }
        LocalFree(wide);
        return arguments;
    }
#else
    // POSIX: аргументы уже в UTF-8 (байты ОС) / POSIX: the arguments are already in UTF-8 (the OS bytes)
    std::vector<std::string> programArguments(int argc, char* argv[])
    {
        return std::vector<std::string>(argv, argv + argc);
    }
#endif
}
