#include "gran/log/log.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#if defined(_WIN32)
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#else
#   include <sys/utsname.h>
#endif

namespace gran::log
{
#if defined(_WIN32)
    /*
    ** Windows: %LOCALAPPDATA%\<программа>\logs — данные программы, которые не переезжают с профилем.
    ** Переменная читается в UTF-16 (у имени пользователя может быть кириллица); нет её — временная папка.
    **
    ** Windows: %LOCALAPPDATA%\<program>\logs — the program data that does not roam with the profile.
    ** The variable is read in UTF-16 (the user name may have Cyrillic); without it — the temporary folder.
    */
    std::string defaultDirectory(const std::string& appName)
    {
        const wchar_t* local = _wgetenv(L"LOCALAPPDATA");
        std::filesystem::path base = std::filesystem::temp_directory_path();

        if (local != nullptr && local[0] != L'\0')
            base = std::filesystem::path(local);
        return (base / std::filesystem::u8path(appName) / "logs").u8string();
    }

    // Настоящая версия ОС — из RtlGetVersion (GetVersionEx без манифеста занижает её) / The real OS version — from RtlGetVersion (GetVersionEx without a manifest understates it)
    std::string systemDescription()
    {
        typedef LONG (WINAPI* GetVersionFn)(OSVERSIONINFOW*);
        OSVERSIONINFOW version{};
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        GetVersionFn getVersion = nullptr;

        version.dwOSVersionInfoSize = sizeof(version);
        if (ntdll != nullptr)
            getVersion = reinterpret_cast<GetVersionFn>(reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion")));
        if (getVersion == nullptr || getVersion(&version) != 0)
            return "Windows";
        return "Windows " + std::to_string(version.dwMajorVersion) + "." + std::to_string(version.dwMinorVersion) + "."
            + std::to_string(version.dwBuildNumber);
    }
#else
    namespace
    {
        // PRETTY_NAME из /etc/os-release («Astra Linux 1.7 x86-64»); нет файла — пусто / PRETTY_NAME from /etc/os-release; no file — empty
        std::string distribution()
        {
            std::ifstream input("/etc/os-release");
            std::string line;

            while (std::getline(input, line))
            {
                if (line.compare(0, 12, "PRETTY_NAME=") != 0)
                    continue;
                line.erase(0, 12);
                if (line.size() >= 2 && line.front() == '"' && line.back() == '"')
                    line = line.substr(1, line.size() - 2);
                return line;
            }
            return std::string();
        }
    }

    // Linux: $XDG_STATE_HOME (по умолчанию ~/.local/state)/<программа>/logs; без HOME — /tmp / Linux: $XDG_STATE_HOME (by default ~/.local/state)/<program>/logs; without HOME — /tmp
    std::string defaultDirectory(const std::string& appName)
    {
        const char* state = std::getenv("XDG_STATE_HOME");
        const char* home = std::getenv("HOME");
        std::filesystem::path base = "/tmp";

        if (state != nullptr && state[0] == '/')
            base = state;
        else if (home != nullptr && home[0] != '\0')
            base = std::filesystem::path(home) / ".local" / "state";
        return (base / appName / "logs").string();
    }

    // Ядро и архитектура из uname, дистрибутив — из os-release / The kernel and the architecture from uname, the distribution — from os-release
    std::string systemDescription()
    {
        struct utsname name{};
        std::string text = "Linux";
        std::string release = distribution();

        if (uname(&name) == 0)
            text = std::string(name.sysname) + " " + name.release + " " + name.machine;
        if (!release.empty())
            text += " / " + release;
        return text;
    }
#endif
}
