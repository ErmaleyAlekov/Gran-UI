#include "gran/settings/internal/settings_xml.hpp"
#include "gran/core/i18n.hpp"
#include "gran/log/log.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace gran::settings
{
    namespace
    {
        // Весь файл в строку; false — файла нет или он не читается / The whole file into a string; false — no file or it is unreadable
        bool readText(const std::filesystem::path& path, std::string& text)
        {
            std::ifstream input(path, std::ios::binary);
            std::ostringstream content;

            if (!input)
                return false;
            content << input.rdbuf();
            text = content.str();
            return true;
        }

        // Испорченный файл остаётся рядом (.bad) для разбора, место освобождается для нового / A damaged file stays beside (.bad) for analysis, the place is freed for a new one
        void keepDamaged(const std::filesystem::path& path)
        {
            std::filesystem::path damaged = path;
            std::error_code error;

            damaged += ".bad";
            std::filesystem::remove(damaged, error);
            std::filesystem::rename(path, damaged, error);
        }
    }

    // Windows — %APPDATA%\<программа> (переезжает с профилем), Linux — $XDG_CONFIG_HOME или ~/.config / Windows — %APPDATA%\<program> (roams with the profile), Linux — $XDG_CONFIG_HOME or ~/.config
    std::string defaultPath(const std::string& appName)
    {
        std::filesystem::path base = std::filesystem::temp_directory_path();
#if defined(_WIN32)
        const wchar_t* roaming = _wgetenv(L"APPDATA");
        if (roaming != nullptr && roaming[0] != L'\0')
            base = std::filesystem::path(roaming);
#else
        const char* config = std::getenv("XDG_CONFIG_HOME");
        const char* home = std::getenv("HOME");
        if (config != nullptr && config[0] == '/')
            base = config;
        else if (home != nullptr && home[0] != '\0')
            base = std::filesystem::path(home) / ".config";
#endif
        return (base / std::filesystem::u8path(appName) / "settings.xml").u8string();
    }

    /*
    ** Нет файла — пустые настройки (первый запуск). Испорченный файл — в .bad, предупреждение в журнал,
    ** пустые настройки: программа запускается со значениями по умолчанию, а не падает.
    **
    ** No file — empty settings (the first run). A damaged file goes into .bad, a warning to the log,
    ** empty settings: the program starts with the default values instead of crashing.
    */
    bool Settings::load(const std::string& path)
    {
        std::filesystem::path file = std::filesystem::u8path(path);
        std::string text;
        Model model;

        _path = path;
        _model = Model{};
        _modified = false;
        if (!readText(file, text))
            return true;
        if (internal::readSettings(text, model))
        {
            _model = std::move(model);
            return true;
        }
        keepDamaged(file);
        log::write(log::Level::WARNING, "settings", i18n::format(i18n::Message::SETTINGS_DAMAGED, {path}));
        return false;
    }

    // Временный файл и переименование: сбой посреди записи не портит прежние настройки / A temporary file and renaming: a failure in the middle of writing does not damage the former settings
    bool Settings::save()
    {
        std::filesystem::path file = std::filesystem::u8path(_path);
        std::filesystem::path temporary = file;
        std::error_code error;

        if (_path.empty())
            return false;
        temporary += ".tmp";
        std::filesystem::create_directories(file.parent_path(), error);
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            output << internal::writeSettings(_model);
            if (!output)
                return false;
        }
        std::filesystem::rename(temporary, file, error);
        if (error)
        {
            std::filesystem::remove(temporary, error);
            return false;
        }
        _modified = false;
        return true;
    }
}
