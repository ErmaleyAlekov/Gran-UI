#ifndef CHAT_PLATFORM_UTF8_ARGUMENTS_HPP
#define CHAT_PLATFORM_UTF8_ARGUMENTS_HPP

#include <string>
#include <vector>

namespace chat
{
    /*
    ** Аргументы командной строки в UTF-8. В Windows argv приходит в кодировке ANSI (cp1251),
    ** и кириллица в --ask превратилась бы в «????»; поэтому строка берётся в UTF-16
    ** (GetCommandLineW) и переводится в UTF-8.
    **
    ** The command line arguments in UTF-8. On Windows argv arrives in the ANSI code page (cp1251),
    ** and Cyrillic in --ask would turn into "????"; so the line is taken in UTF-16
    ** (GetCommandLineW) and converted to UTF-8.
    */
    std::vector<std::string> utf8Arguments();
}

#endif
