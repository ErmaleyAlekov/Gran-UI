#include "platform/utf8_arguments.hpp"

#include "gran/platform/win32/win32_window.hpp"

#include <shellapi.h>
#include <memory>

namespace chat
{
    // Разбор строки по правилам Windows (кавычки, обратные косые) — CommandLineToArgvW / Splitting the line by the Windows rules (quotes, backslashes) — CommandLineToArgvW
    std::vector<std::string> utf8Arguments()
    {
        std::vector<std::string> arguments;
        int count = 0;
        std::unique_ptr<LPWSTR[], decltype(&LocalFree)> wide(CommandLineToArgvW(GetCommandLineW(), &count), LocalFree);

        if (wide == nullptr)
            return arguments;
        for (int index = 0; index < count; ++index)
            arguments.push_back(gran::platform::win32::toUtf8(wide[index]));
        return arguments;
    }
}
