#ifndef GRAN_UTILS_PROGRAM_ARGUMENTS_HPP
#define GRAN_UTILS_PROGRAM_ARGUMENTS_HPP

#include <string>
#include <vector>

namespace gran
{
    /*
    ** Аргументы командной строки в UTF-8 на всех ОС. На Windows argv в main приходит в кодовой странице ANSI:
    ** путь «C:\Проба 日本\a.xml» превращается в «C:\Проба ??\a.xml» и не открывается (найдено проверкой Т-56) —
    ** поэтому строка берётся заново из GetCommandLineW и разбирается CommandLineToArgvW. На Linux и macOS argv —
    ** байты, которые ОС передаёт как есть (UTF-8), и функция просто копирует их.
    **
    ** The command line arguments in UTF-8 on every OS. On Windows argv in main comes in the ANSI code page:
    ** the "C:\Проба 日本\a.xml" path turns into "C:\Проба ??\a.xml" and does not open (found by the T-56 check) —
    ** so the line is taken again from GetCommandLineW and parsed by CommandLineToArgvW. On Linux and macOS argv is
    ** bytes the OS passes as is (UTF-8), and the function just copies them.
    */
    std::vector<std::string> programArguments(int argc, char* argv[]);
}

#endif
